/*-----------------------------------Includes-----------------------------------*/
#include <Arduino.h>
#include <WiFi.h>
#include <Ticker.h>
#include <TFT_eSPI.h>
#include <freertos/FreeRTOS.h>
#include <driver/adc.h>
#include <esp32-hal-psram.h>
#include <UrlEncode.h>
#include <ArduinoNVS.h>

#include "GetData.h"
#include "VariableDeclaration.h"
#include "WiFiHandle.h"
#include "Display.h"
#include "Menu.h"

using namespace fs;

char name[50], display_name[50];
char ssid[56], pass[56], raw_address[64];
double lat, lon;
int gmtOffset_sec = 0;
uint8_t aqi;
double uv;
bool newScreen = false;

struct tm structTime;

TaskHandle_t WorkingFlowControl_Handle;
TaskHandle_t WiFi_Handle;
TaskHandle_t GetCurrentWeather_Handle;
TaskHandle_t GetThreeHoursForecast_Handle;
// TaskHandle_t GetForecastWeather_Handle;
TaskHandle_t GetAQI_Handle;
TaskHandle_t GetUV_Handle;
TaskHandle_t ProcessingCurrentWeather_Handle;
// TaskHandle_t ProcessingForecastWeather_Handle;
TaskHandle_t ProcessingThreeHoursForecast_Handle;
TaskHandle_t DisplayTitle_Handle;
TaskHandle_t DisplayCurrentWeather_Handle;
// TaskHandle_t DisplayForecastWeather_Handle;
TaskHandle_t DisplayThreeHoursForecast_Handle;
TaskHandle_t ButtonMonitoring_Handle;
TaskHandle_t MenuControl_Handle;

Ticker GetCurrentWeather_Ticker;
Ticker GetForeCastWeather_Ticker;

EventGroupHandle_t WorkingFlow_EventGroup = xEventGroupCreate();
EventGroupHandle_t CurrentFlow_EventGroup = xEventGroupCreate();
EventGroupHandle_t WiFi_EventGroup = xEventGroupCreate();
EventGroupHandle_t GetData_EventGroup = xEventGroupCreate();

// SemaphoreHandle_t http_mutex;

QueueHandle_t current_weather_queue;
// QueueHandle_t forecast_queue = xQueueCreate(8, sizeof(forecast) + 1);
QueueHandle_t three_hours_forecast_queue;

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite title_Sprite = TFT_eSprite(&tft);
TFT_eSprite current_weather_Sprite = TFT_eSprite(&tft);
TFT_eSprite Menu_Sprite = TFT_eSprite(&tft);
TFT_eSprite menu_cursor_Sprite = TFT_eSprite(&tft);
TFT_eSprite forecast3hours_weather_Sprite = TFT_eSprite(&tft);

HTTPClient http;

/*---------------------------------Prototypes-----------------------------------*/
void WorkingFlowControl(void *pvParameters);
void startGetCurrentWeather();
void startGetForecastWeather();
void getLocation(char *raw_address, double &lat, double &lon);
void getTimeZone(double lat, double lon, int &gmtOffset_sec);
void setup()
{
	// put your setup code here, to run once:
	pinMode(KEY, INPUT_PULLUP);
	pinMode(UP_KEY, INPUT_PULLUP);
	pinMode(DOWN_KEY, INPUT_PULLUP);
	pinMode(LEFT_KEY, INPUT_PULLUP);
	pinMode(RIGHT_KEY, INPUT_PULLUP);

	Serial.begin(115200);
	Serial.println("System starting...");

	NVS.begin();
	// init psram
	if (!psramFound())
	{
		Serial.println("Error: PSRAM not found!");
		while (true)
			;
	}
	psramInit();
	Serial.println("PSRAM found and initialized!");

	StaticSemaphore_t *xMutexBuffer = (StaticSemaphore_t *)ps_malloc(sizeof(StaticSemaphore_t));
	current_weather_queue = xQueueCreate(1, sizeof(weather_data) + 1);
	three_hours_forecast_queue = xQueueCreate(7, sizeof(forecast));

	// http_mutex = xSemaphoreCreateMutexStatic(xMutexBuffer);

	tft.init();
	tft.initDMA();
	tft.setTextWrap(true, true);
	tft.fillScreen(TFT_BLACK);
	tft.setCursor(0, 0);
	tft.setRotation(2);
	tft.println("System starting...");
	delay(500);

	// mount FFat
	if (!FFat.begin())
	{
		Serial.println("FFat Mount Failed");
		ESP.restart();
		return;
	}

	tft.loadFont("Calibri-Bold-11", FFat);
	Serial.println("FFat Mount Success");
	tft.println("FFat Mount Success");
	delay(500);

	// connect to WiFi
	Serial.println("Connecting to WiFi...");
	tft.println("Connecting to WiFi...");
	connectToWiFi(); // use for get WiFi information
	Serial.println("WiFi connected to " + String(ssid));
	tft.println("WiFi connected to " + String(ssid));
	delay(500);

	// get coordinates from address
	Serial.println("Getting location...");
	tft.println("Getting location...");
	if (NVS.getString("address") == "") // if address is not stored in NVS, get address from user
	{
		uint8_t mac[6];
		WiFi.macAddress(mac);
		char ap_ssid[32];
		sprintf(ap_ssid, "Weather Station %02X%02X%02X", mac[3], mac[4], mac[5]);
		inputLocation(ap_ssid, "");
		tft.println("Please input your location");
		tft.printf("Connect to WiFi: %s\n", ap_ssid);
		tft.printf("Access to this IP: %s\n", WiFi.softAPIP().toString().c_str());
		while (true)
		{
			server.handleClient();
			if (server.hasArg("address"))
			{
				server.arg("address").toCharArray(raw_address, sizeof(raw_address));
				server.stop();
				NVS.setString("address", raw_address);
				WiFi.mode(WIFI_STA); // return to normal mode
				WiFi.begin(ssid, pass);
				break;
			}
		}
	}
	else
	{
		NVS.getString("address").toCharArray(raw_address, sizeof(raw_address));
	}
	Serial.printf("Address: %s\n", raw_address);

	getLocation(raw_address, lat, lon); // get coordinates from GPS module
	Serial.printf("Coordinates: %.6f, %.6f \n", lat, lon);
	Serial.printf("City name: %s\n", name);

	// getting time zone
	Serial.println("Getting time zone...");
	tft.println("Getting time zone...");
	getTimeZone(lat, lon, gmtOffset_sec);
	delay(500);

	// sync time with the internet
	configTime(gmtOffset_sec, daylightOffset_sec, ntpServer); // sync time with the internet
	tft.print("Waiting for time to sync...");
	Serial.println("Waiting for time to sync...");
	while (!getLocalTime(&structTime, 500))
	{
		delay(10);
	}
	Serial.println("Time synced");
	tft.println("Time synced");
	delay(500);

	// create title sprite and load font
	title_Sprite.setColorDepth(16);
	title_Sprite.createSprite(240, 11);
	title_Sprite.loadFont("Calibri-Bold-11", FFat);

	// create current weather sprite and load font
	current_weather_Sprite.setColorDepth(16);
	current_weather_Sprite.createSprite(160, 150);
	current_weather_Sprite.loadFont("Calibri-Bold-11", FFat);

	// create menu sprite and load font
	Menu_Sprite.setColorDepth(16);
	Menu_Sprite.createSprite(225, 320);
	Menu_Sprite.loadFont("Calibri-Bold-11", FFat);

	// create forecast 3 hours sprite and load font
	forecast3hours_weather_Sprite.setColorDepth(16);
	forecast3hours_weather_Sprite.createSprite(120, 100);
	forecast3hours_weather_Sprite.loadFont("Calibri-Bold-11", FFat);

	// create menu cursor sprite
	menu_cursor_Sprite.setColorDepth(16);
	menu_cursor_Sprite.createSprite(15, 320);

	// control working flow
	xTaskCreatePinnedToCore(WorkingFlowControl, "Working Flow Control", 5120, NULL, 10, &WorkingFlowControl_Handle, 0);
	// wifi handle task
	xTaskCreatePinnedToCore(HandleWiFi, "WiFi Handle", 3072, NULL, 2, &WiFi_Handle, 0);
	// get data task
	xTaskCreatePinnedToCore(GetCurrentWeather, "Get Current Weather", 4096, NULL, 1, &GetCurrentWeather_Handle, 0);
	// xTaskCreatePinnedToCore(GetForecastWeather, "Get Forecast Weather", 8192, NULL, 1, &GetForecastWeather_Handle, 0);
	xTaskCreatePinnedToCore(GetAQI, "Get AQI", 3072, NULL, 1, &GetAQI_Handle, 0);
	xTaskCreatePinnedToCore(GetUV, "Get UV", 3072, NULL, 1, &GetUV_Handle, 0);
	xTaskCreatePinnedToCore(Get3_HoursForecastWeather, "Get 3 hours forecast", 10000, NULL, 1, &GetThreeHoursForecast_Handle, 0);
	// processing task
	xTaskCreatePinnedToCore(ProcessingCurrentWeather, "Processing Current Weather", 5120, NULL, 1, &ProcessingCurrentWeather_Handle, 0);
	// xTaskCreatePinnedToCore(ProcessingForecastWeather, "Processing Forecast Weather", 5120, NULL, 1, &ProcessingForecastWeather_Handle, 0);
	xTaskCreatePinnedToCore(Processing3_HoursForecastWeather, "Processing 3 hours forecast", 5120, NULL, 1, &ProcessingThreeHoursForecast_Handle, 0);
	// display task
	xTaskCreatePinnedToCore(DisplayTitle, "Display Title", 2048, NULL, 1, &DisplayTitle_Handle, 1);
	xTaskCreatePinnedToCore(DisplayCurrentWeather, "Display Current Weather", 4096, NULL, 1, &DisplayCurrentWeather_Handle, 1);
	// xTaskCreatePinnedToCore(DisplayForecastWeather, "Display Forecast Weather", 5120, NULL, 1, &DisplayForecastWeather_Handle, 1);
	xTaskCreatePinnedToCore(DisplayThreeHoursForecast, "Display 3 hours forecast", 4096, NULL, 1, &DisplayThreeHoursForecast_Handle, 1);
	// check button state
	xTaskCreatePinnedToCore(ButtonMonitoring, "Button Monitoring", 2560, NULL, 2, &ButtonMonitoring_Handle, 0);
	xTaskCreatePinnedToCore(MenuControl, "Menu Control", 4096, NULL, 1, &MenuControl_Handle, 1);

	// Serial.println("Suspending all tasks");
	vTaskSuspend(GetCurrentWeather_Handle);
	// vTaskSuspend(GetForecastWeather_Handle);
	vTaskSuspend(GetAQI_Handle);
	vTaskSuspend(GetUV_Handle);
	vTaskSuspend(ProcessingCurrentWeather_Handle);
	// vTaskSuspend(ProcessingForecastWeather_Handle);
	vTaskSuspend(DisplayTitle_Handle);

	Serial.println("Finished setup");
	tft.println("Finished setup");
	delay(500);
	// tft.unloadFont();

	// xEventGroupSetBits(GetData_EventGroup, START_GET_CURRENT_WEATHER_FLAG | START_GET_AQI_FLAG | START_GET_UV_FLAG); // first run
	xEventGroupSetBits(GetData_EventGroup, START_GET_CURRENT_WEATHER_FLAG | START_GET_AQI_FLAG); // first run
	xEventGroupSetBits(WorkingFlow_EventGroup, MAIN);											 // first run

	GetCurrentWeather_Ticker.attach(50, startGetCurrentWeather); // trigger Get Current Weather every 1 hour
	// GetForeCastWeather_Ticker.attach(24 * 3600 * 7, startGetForecastWeather); // trigger Get Forecast Weather every 7 days
}

void loop()
{
	// put your main code here, to run repeatedly:
}

void WorkingFlowControl(void *pvParameters)
{
	EventBits_t event;
	for (;;)
	{
		event = xEventGroupWaitBits(WorkingFlow_EventGroup, MAIN | MENU | THREE_HOURS_FORECAST | INPUT_API | INPUT_WIFI_CREDENTIALS | CHANGE_LOCATION, pdTRUE, pdFALSE, portMAX_DELAY);
		switch (event)
		{
		case MAIN:
			if (xEventGroupGetBits(CurrentFlow_EventGroup) == MAIN)
			{
				break;
			}
			tft.fillScreen(TFT_BLACK);
			vTaskResume(GetCurrentWeather_Handle);
			// vTaskResume(GetForecastWeather_Handle);
			vTaskResume(GetAQI_Handle);
			vTaskResume(GetUV_Handle);
			vTaskResume(ProcessingCurrentWeather_Handle);
			// vTaskResume(ProcessingForecastWeather_Handle);
			vTaskResume(DisplayTitle_Handle);
			vTaskResume(DisplayCurrentWeather_Handle);
			// vTaskResume(DisplayForecastWeather_Handle);
			xEventGroupSetBits(CurrentFlow_EventGroup, MAIN);
			break;
		/* ----------------------- */
		case MENU:
			if (xEventGroupGetBits(CurrentFlow_EventGroup) == MENU)
			{
				break;
			}
			tft.fillScreen(TFT_BLACK);
			vTaskSuspend(DisplayTitle_Handle);
			vTaskSuspend(DisplayCurrentWeather_Handle);
			// vTaskSuspend(DisplayForecastWeather_Handle);

			// resume to finish task and prevent infinitive waiting
			vTaskResume(GetCurrentWeather_Handle);
			// vTaskResume(GetForecastWeather_Handle);
			vTaskResume(GetAQI_Handle);
			vTaskResume(GetUV_Handle);
			vTaskResume(ProcessingCurrentWeather_Handle);
			// vTaskResume(ProcessingForecastWeather_Handle);

			tft.setTextDatum(MC_DATUM);
			tft.drawString("Please wait for task to finish", tft.width() / 2, 40, 0);
			tft.setTextDatum(TL_DATUM);

			while (xEventGroupGetBits(GetData_EventGroup) & (START_GET_CURRENT_WEATHER_FLAG | START_GET_AQI_FLAG | START_GET_UV_FLAG))
			{
				delay(100);
			}
			tft.fillScreen(TFT_BLACK);

			vTaskSuspend(GetCurrentWeather_Handle);
			// vTaskSuspend(GetForecastWeather_Handle);
			vTaskSuspend(GetAQI_Handle);
			vTaskSuspend(GetUV_Handle);
			vTaskSuspend(ProcessingCurrentWeather_Handle);
			// vTaskSuspend(ProcessingForecastWeather_Handle);
			displayMenu();
			xEventGroupSetBits(CurrentFlow_EventGroup, MENU);
			break;
		/* ----------------------- */
		case THREE_HOURS_FORECAST:
			if (xEventGroupGetBits(CurrentFlow_EventGroup) == THREE_HOURS_FORECAST)
			{
				break;
			}
			tft.fillScreen(TFT_BLACK);
			xEventGroupSetBits(GetData_EventGroup, START_GET_3HOURS_FORECAST_FLAG);


			xEventGroupSetBits(CurrentFlow_EventGroup, THREE_HOURS_FORECAST);
			break;
		/* ----------------------- */
		case CHANGE_LOCATION:
			if (xEventGroupGetBits(CurrentFlow_EventGroup) == CHANGE_LOCATION)
			{
				break;
			}
			tft.fillScreen(TFT_BLACK);
			tft.setCursor(0, 0);
			uint8_t mac[6];
			WiFi.macAddress(mac);
			char ap_ssid[32];
			sprintf(ap_ssid, "Weather Station %02X%02X%02X", mac[3], mac[4], mac[5]);
			inputLocation(ap_ssid, "");
			tft.println("Please input your new location");
			tft.printf("Connect to WiFi: %s\n", ap_ssid);
			tft.printf("Access to this link: %s/location\n", WiFi.softAPIP().toString().c_str());
			while (true)
			{
				server.handleClient();
				if (server.hasArg("address"))
				{
					server.arg("address").toCharArray(raw_address, sizeof(raw_address));
					NVS.setString("address", raw_address);
					server.stop();
					WiFi.softAPdisconnect(true);
					WiFi.mode(WIFI_OFF);
					break;
				}
			}
			delay(2);
			xEventGroupSetBits(GetData_EventGroup, GET_LOCATION_FLAG);
			xEventGroupSetBits(WiFi_EventGroup, REQUEST_WIFI_FLAG);
			xEventGroupWaitBits(WiFi_EventGroup, WIFI_IS_AVAILABLE_FOR_USE_FLAG, pdFALSE, pdFALSE, portMAX_DELAY);
			getLocation(raw_address, lat, lon);						  // call to get new location
			getTimeZone(lat, lon, gmtOffset_sec);					  // call to get new time zone
			configTime(gmtOffset_sec, daylightOffset_sec, ntpServer); // resync time
			tft.println("Waiting for time to resync...");
			Serial.println("Waiting for time to resync...");
			while (!getLocalTime(&structTime, 500))
			{
				delay(10);
			}
			xEventGroupSetBits(WiFi_EventGroup, DONE_USING_WIFI_FLAG);
			xEventGroupSetBits(CurrentFlow_EventGroup, CHANGE_LOCATION);
			xEventGroupClearBits(CurrentFlow_EventGroup, 0xFFFF);
			// xEventGroupSetBits(GetData_EventGroup, START_GET_CURRENT_WEATHER_FLAG | START_GET_AQI_FLAG | START_GET_UV_FLAG);
			newScreen = true; // force to update to new screen
			GetCurrentWeather_Ticker.attach(10, startGetCurrentWeather); // reset timer
			xEventGroupSetBits(GetData_EventGroup, START_GET_CURRENT_WEATHER_FLAG | START_GET_AQI_FLAG);
			xEventGroupSetBits(WorkingFlow_EventGroup, MAIN);
			break;
		/* ----------------------- */
		case INPUT_API:
			xEventGroupSetBits(CurrentFlow_EventGroup, INPUT_API);
			break;
		/* ----------------------- */
		default:
			break;
		}
		taskYIELD();
	}
}

void startGetCurrentWeather()
{
	Serial.println("Interrupt");
	BaseType_t xHigherPriorityTaskWoken, xResult;
	xHigherPriorityTaskWoken = pdFALSE;
	// xResult = xEventGroupSetBitsFromISR(GetData_EventGroup, START_GET_CURRENT_WEATHER_FLAG | START_GET_AQI_FLAG | START_GET_UV_FLAG, &xHigherPriorityTaskWoken);
	xResult = xEventGroupSetBitsFromISR(GetData_EventGroup, START_GET_CURRENT_WEATHER_FLAG | START_GET_AQI_FLAG, &xHigherPriorityTaskWoken);

	if (xResult != pdFAIL)
	{
		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	}
}

void startGetForecastWeather()
{
	// Serial.println("Get Forecast Weather Start");
	BaseType_t xHigherPriorityTaskWoken, xResult;
	xHigherPriorityTaskWoken = pdFALSE;
	xResult = xEventGroupSetBitsFromISR(GetData_EventGroup, START_GET_FORECAST_WEATHER_FLAG, &xHigherPriorityTaskWoken);

	if (xResult != pdFAIL)
	{
		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	}
}

void getLocation(char *raw_address, double &lat, double &lon)
{
	PSRAMJsonDocument doc(4096);
	char url[2048];
	char address[512];
	urlEncode(raw_address).toCharArray(address, sizeof(address));
	snprintf(url, sizeof(url), "http://api.opencagedata.com/geocode/v1/json?q=%s&key=%s&limit=1", address, opencage_api_key);
	// Serial.println(url);
	http.begin(url);
	while (http.GET() != HTTP_CODE_OK)
	{
		delay(100);
	}
	deserializeJson(doc, http.getString());
	// Serial.println(doc.as<String>());
	http.end();
	lat = doc["results"][0]["geometry"]["lat"].as<double>();
	lon = doc["results"][0]["geometry"]["lng"].as<double>();

	if (!doc["results"][0]["components"].containsKey("city"))
	{
		doc["results"][0]["components"]["county"].as<String>().toCharArray(name, sizeof(name));
		strcpy(display_name, name); // copy name to display_city_name (for display purpose
		// remove the word "City" in the city name to ensure the title fit the screen
		char *pos = strstr(display_name, " District"); // copy name to display_city_name (for display purpose)
		if (pos != nullptr)
		{
			memmove(pos, pos + strlen(" District"), strlen(pos + strlen(" District")) + 1);
		}
	}
	else
	{
		doc["results"][0]["components"]["city"].as<String>().toCharArray(name, sizeof(name));
		strcpy(display_name, name); // copy name to display_city_name (for display purpose
		// remove the word "City" in the city name to ensure the title fits the screen
		char *pos = strstr(display_name, " City"); // copy name to display_city_name (for display purpose)
		if (pos != nullptr)
		{
			memmove(pos, pos + strlen(" City"), strlen(pos + strlen(" City")) + 1);
		}
	}
	xEventGroupClearBits(GetData_EventGroup, GET_LOCATION_FLAG);
}

void getTimeZone(double lat, double lon, int &gmtOffset_sec)
{
	PSRAMJsonDocument doc(1024);
	char time_zone_url[128];
	snprintf(time_zone_url, sizeof(time_zone_url), "http://api.timezonedb.com/v2.1/get-time-zone?key=%s&format=json&by=position&lat=%.6f&lng=%.6f", timezone_api_key, lat, lon);
	http.begin(time_zone_url);
	while (http.GET() != HTTP_CODE_OK)
	{
		delay(100);
	}
	deserializeJson(doc, http.getString());
	http.end();
	gmtOffset_sec = doc["gmtOffset"].as<int>();
}

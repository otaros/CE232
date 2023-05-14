/*-----------------------------------Includes-----------------------------------*/
#include <Arduino.h>
#include <WiFi.h>
#include <Ticker.h>
#include <TFT_eSPI.h>
#include <freertos/FreeRTOS.h>
#include <driver/adc.h>
#include <esp32-hal-psram.h>
#include <UrlEncode.h>

#include "GetData.h"
#include "VariableDeclaration.h"
#include "WiFiHandle.h"
#include "Display.h"
#include "Menu.h"

using namespace fs;

char name[50], display_name[50];
char ssid[56], pass[56];
double lat, lon;
int gmtOffset_sec = 0;
uint8_t aqi;
double uv;

struct tm structTime;

// TinyGPSPlus gps;

TaskHandle_t WorkingFlowControl_Handle;
TaskHandle_t WiFi_Handle;
TaskHandle_t GetCurrentWeather_Handle;
TaskHandle_t ProcessingCurrentWeather_Handle;
TaskHandle_t GetForecastWeather_Handle;
TaskHandle_t ProcessingForecastWeather_Handle;
TaskHandle_t DisplayTitle_Handle;
TaskHandle_t DisplayCurrentWeather_Handle;
TaskHandle_t DisplayForecastWeather_Handle;
TaskHandle_t GetAQI_Handle;
TaskHandle_t GetUV_Handle;
TaskHandle_t ButtonMonitoring_Handle;
TaskHandle_t MenuControl_Handle;

Ticker GetCurrentWeather_Ticker;
Ticker GetForeCastWeather_Ticker;

EventGroupHandle_t WorkingFlow_EventGroup = xEventGroupCreate();
EventGroupHandle_t CurrentFlow_EventGroup = xEventGroupCreate();
EventGroupHandle_t WiFi_EventGroup = xEventGroupCreate();
EventGroupHandle_t GetData_EventGroup = xEventGroupCreate();

SemaphoreHandle_t coordinate_mutex = xSemaphoreCreateMutex();
SemaphoreHandle_t http_mutex = xSemaphoreCreateMutex();

QueueHandle_t current_weather_queue = xQueueCreate(2, sizeof(weather_data));
QueueHandle_t forecast_queue = xQueueCreate(8, sizeof(forecast));

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite title_Sprite = TFT_eSprite(&tft);
TFT_eSprite current_weather_Sprite = TFT_eSprite(&tft);
TFT_eSprite Menu_Sprite = TFT_eSprite(&tft);
TFT_eSprite menu_cursor_Sprite = TFT_eSprite(&tft);

HTTPClient http;

/*---------------------------------Prototypes-----------------------------------*/
void WorkingFlowControl(void *pvParameters);
void startGetCurrentWeather();
void startGetForecastWeather();
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

	// init psram
	if (!psramFound())
	{
		Serial.println("Error: PSRAM not found!");
		while (true)
			;
	}
	psramInit();
	Serial.println("PSRAM found and initialized!");

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

	// get coordinates from GPS module
	Serial.println("Getting location...");
	tft.println("Getting location...");
	getLocation(); // get coordinates from GPS module
	Serial.printf("Coordinates: %.6f, %.6f \n", lat, lon);
	Serial.printf("City name: %s\n", name);

	PSRAMJsonDocument doc(1024);

	// getting time zone
	char time_zone_url[128];
	Serial.println("Getting time zone...");
	tft.println("Getting time zone...");
	snprintf(time_zone_url, sizeof(time_zone_url), "http://api.timezonedb.com/v2.1/get-time-zone?key=%s&format=json&by=position&lat=%.6f&lng=%.6f", timezone_api_key, lat, lon);
	http.begin(time_zone_url);
	while (http.GET() != HTTP_CODE_OK)
	{
		delay(100);
	}
	deserializeJson(doc, http.getString());
	http.end();
	gmtOffset_sec = doc["gmtOffset"].as<int>();
	delay(500);

	// sync time with the internet
	configTime(gmtOffset_sec, daylightOffset_sec, ntpServer); // sync time with the internet
	tft.print("Waiting for time to sync...");
	Serial.print("Waiting for time to sync...");
	while (!getLocalTime(&structTime, 500))
	{
		delay(10);
	}
	Serial.println("");
	tft.println("");
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

	// create menu cursor sprite
	menu_cursor_Sprite.setColorDepth(16);
	menu_cursor_Sprite.createSprite(15, 320);

	GetCurrentWeather_Ticker.attach(3600, startGetCurrentWeather);			  // trigger Get Current Weather every 1 hour
	GetForeCastWeather_Ticker.attach(24 * 3600 * 7, startGetForecastWeather); // trigger Get Forecast Weather every 7 days

	// control working flow
	xTaskCreatePinnedToCore(WorkingFlowControl, "WorkingFlowControl", 2048, NULL, 2, &WorkingFlowControl_Handle, 0);
	// wifi handle task
	xTaskCreatePinnedToCore(HandleWiFi, "WiFi_Handle", 3072, NULL, 2, &WiFi_Handle, 0);
	// get data task
	xTaskCreatePinnedToCore(GetCurrentWeather, "GetCurrentWeather", 4096, NULL, 1, &GetCurrentWeather_Handle, 0);
	xTaskCreatePinnedToCore(GetForecastWeather, "GetForecastWeather", 8192, NULL, 1, &GetForecastWeather_Handle, 0);
	xTaskCreatePinnedToCore(GetAQI, "GetAQI", 3072, NULL, 1, &GetAQI_Handle, 0);
	xTaskCreatePinnedToCore(GetUV, "GetUV", 3072, NULL, 1, &GetUV_Handle, 0);
	// processing task
	xTaskCreatePinnedToCore(ProcessingCurrentWeather, "ProcessingCurrentWeather", 5120, NULL, 1, &ProcessingCurrentWeather_Handle, 0);
	xTaskCreatePinnedToCore(ProcessingForecastWeather, "ProcessingForecastWeather", 5120, NULL, 1, &ProcessingForecastWeather_Handle, 0);
	// display task
	xTaskCreatePinnedToCore(DisplayTitle, "Display Title", 2048, NULL, 1, &DisplayTitle_Handle, 1);
	xTaskCreatePinnedToCore(DisplayCurrentWeather, "Display Current Weather", 4096, NULL, 1, &DisplayCurrentWeather_Handle, 1);
	xTaskCreatePinnedToCore(DisplayForecastWeather, "Display Forecast Weather", 5120, NULL, 1, &DisplayForecastWeather_Handle, 1);
	// check button state
	xTaskCreatePinnedToCore(ButtonMonitoring, "Button Monitoring", 2048, NULL, 1, &ButtonMonitoring_Handle, 1);
	xTaskCreatePinnedToCore(MenuControl, "Menu Control", 2048, NULL, 1, &MenuControl_Handle, 1);

	// Serial.println("Suspending all tasks");
	vTaskSuspend(GetCurrentWeather_Handle);
	vTaskSuspend(GetForecastWeather_Handle);
	vTaskSuspend(GetAQI_Handle);
	vTaskSuspend(GetUV_Handle);
	vTaskSuspend(ProcessingCurrentWeather_Handle);
	vTaskSuspend(ProcessingForecastWeather_Handle);
	vTaskSuspend(DisplayTitle_Handle);

	Serial.println("Finished setup");
	tft.println("Finished setup");
	delay(500);
	tft.fillScreen(TFT_BLACK); // clear display
	// tft.unloadFont();

	xEventGroupSetBits(WorkingFlow_EventGroup, MAIN); // first run
	// xEventGroupSetBits(GetData_EventGroup, START_GET_CURRENT_WEATHER_FLAG | START_GET_AQI_FLAG | START_GET_UV_FLAG); // first run
	xEventGroupSetBits(GetData_EventGroup, START_GET_CURRENT_WEATHER_FLAG | START_GET_AQI_FLAG); // first run
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
		event = xEventGroupWaitBits(WorkingFlow_EventGroup, MAIN | MENU | THREE_HOURS_FORECAST | INPUT_API | INPUT_WIFI_CREDENTIALS, pdTRUE, pdFALSE, portMAX_DELAY);
		switch (event)
		{
		case MAIN:
			if (xEventGroupGetBits(CurrentFlow_EventGroup) == MAIN)
			{
				break;
			}
			tft.fillScreen(TFT_BLACK);
			vTaskResume(GetCurrentWeather_Handle);
			vTaskResume(GetForecastWeather_Handle);
			vTaskResume(GetAQI_Handle);
			vTaskResume(GetUV_Handle);
			vTaskResume(ProcessingCurrentWeather_Handle);
			vTaskResume(ProcessingForecastWeather_Handle);
			vTaskResume(DisplayTitle_Handle);
			vTaskResume(DisplayCurrentWeather_Handle);
			vTaskResume(DisplayForecastWeather_Handle);
			xEventGroupSetBits(CurrentFlow_EventGroup, MAIN);
			break;
		case MENU:
			if (xEventGroupGetBits(CurrentFlow_EventGroup) == MENU)
			{
				break;
			}
			tft.fillScreen(TFT_BLACK);
			vTaskSuspend(DisplayTitle_Handle);
			vTaskSuspend(DisplayCurrentWeather_Handle);
			vTaskSuspend(DisplayForecastWeather_Handle);

			tft.println("Please wait for task to finish");
			while (xEventGroupGetBits(GetData_EventGroup) & (START_GET_CURRENT_WEATHER_FLAG | START_GET_AQI_FLAG | START_GET_UV_FLAG))
			{
				delay(100);
			}
			tft.fillScreen(TFT_BLACK);

			vTaskSuspend(GetCurrentWeather_Handle);
			vTaskSuspend(GetForecastWeather_Handle);
			vTaskSuspend(GetAQI_Handle);
			vTaskSuspend(GetUV_Handle);
			vTaskSuspend(ProcessingCurrentWeather_Handle);
			vTaskSuspend(ProcessingForecastWeather_Handle);
			displayMenu();
			xEventGroupSetBits(CurrentFlow_EventGroup, MENU);
			break;
		default:
			break;
		}
	}
}

void startGetCurrentWeather()
{
	Serial.println("Get Current Weather Start");
	BaseType_t xHigherPriorityTaskWoken, xResult;
	xHigherPriorityTaskWoken = pdFALSE;
	xResult = xEventGroupSetBitsFromISR(GetData_EventGroup, START_GET_CURRENT_WEATHER_FLAG | START_GET_AQI_FLAG | START_GET_UV_FLAG, &xHigherPriorityTaskWoken);

	if (xResult != pdFAIL)
	{
		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	}
}

void startGetForecastWeather()
{
	Serial.println("Get Forecast Weather Start");
	BaseType_t xHigherPriorityTaskWoken, xResult;
	xHigherPriorityTaskWoken = pdFALSE;
	xResult = xEventGroupSetBitsFromISR(GetData_EventGroup, START_GET_FORECAST_WEATHER_FLAG, &xHigherPriorityTaskWoken);

	if (xResult != pdFAIL)
	{
		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	}
}

void getLocation()
{
	// check NVS data if location is set -> get address from NVS -> get coordinates from address
	// if not set -> openwebserver for user to input address -> store address to NVS -> get coordinates from address
	// if NVS data is invalid -> openwebserver for user to input address -> store address to NVS -> get coordinates from address
	char raw_address[] = "Berlin, Germany";
	PSRAMJsonDocument doc(4096);
	char url[2048];
	char address[512];
	urlEncode(raw_address).toCharArray(address, sizeof(address));
	snprintf(url, sizeof(url), "http://api.opencagedata.com/geocode/v1/json?q=%s&key=%s&limit=1", address, opencage_api_key);
	Serial.println(url);
	http.begin(url);
	while (http.GET() != HTTP_CODE_OK)
	{
		delay(100);
	}
	deserializeJson(doc, http.getString());
	Serial.println(doc.as<String>());
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
}
/*-----------------------------------Includes-----------------------------------*/
#include <Arduino.h>
#include <WiFi.h>
#include <Ticker.h>
#include <TFT_eSPI.h>
#include <freertos/FreeRTOS.h>
#include <driver/adc.h>
#include <esp32-hal-psram.h>

#include "GetData.h"
#include "VariableDeclaration.h"
#include "WiFiHandle.h"
#include "Display.h"

using namespace fs;

char city_name[50], display_name[40];
char ssid[56], pass[56];
double lat, lon;
int gmtOffset_sec = 0; // GMT +7
uint8_t aqi;
double uv;

struct tm structTime;

TinyGPSPlus gps;

TaskHandle_t WiFi_Handle;
TaskHandle_t GetCurrentWeather_Handle;
TaskHandle_t ProcessingCurrentWeather_Handle;
TaskHandle_t GetForecastWeather_Handle;
TaskHandle_t ProcessingForecastWeather_Handle;
TaskHandle_t DisplayTitle_Handle;
TaskHandle_t DisplayCurrentWeather_Handle;
TaskHandle_t GetAQI_Handle;
TaskHandle_t GetUV_Handle;

Ticker GetCurrentWeather_Ticker;
Ticker GetForeCastWeather_Ticker;

EventGroupHandle_t WiFi_EventGroup = xEventGroupCreate();
EventGroupHandle_t GetData_EventGroup = xEventGroupCreate();

SemaphoreHandle_t coordinate_mutex = xSemaphoreCreateMutex();
SemaphoreHandle_t http_mutex = xSemaphoreCreateMutex();

QueueHandle_t current_weather_queue = xQueueCreate(2, sizeof(weather_data));
QueueHandle_t forecast_queue = xQueueCreate(8, sizeof(forecast));

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite title_Sprite = TFT_eSprite(&tft);
TFT_eSprite current_weather_Sprite = TFT_eSprite(&tft);

HTTPClient http;

/*---------------------------------Prototypes-----------------------------------*/
void startGetCurrentWeather();
void startGetForecastWeather();
void getCoordinates();
void setup()
{
	// put your setup code here, to run once:
	Serial.begin(115200);
	Serial.println("System starting...");
	Serial1.begin(9600);
	Serial1.setPins(4, 5); // pin for GPS module

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
	tft.setRotation(1);
	tft.fillScreen(TFT_BLACK);
	tft.setRotation(1);
	tft.setCursor(0, 0);
	tft.println("System starting...");
	delay(500);

	// mount FFat
	if (!FFat.begin(false))
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
	Serial.println("Getting coordinates...");
	getCoordinates(); // get coordinates from GPS module
	Serial.printf("Coordinates: %.6f, %.6f \n", lat, lon);

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

	// getting city name
	Serial.println("Getting city name...");
	tft.println("Getting city name...");
	char location_url[128];
	snprintf(location_url, sizeof(location_url), "http://api.openweathermap.org/geo/1.0/reverse?lat=%.6f&lon=%.6f&limit=1&appid=%s", lat, lon, api_key);
	// Serial.println(location); // for checking purpose
	http.begin(location_url);
	while (http.GET() != HTTP_CODE_OK)
	{
		delay(100);
	}
	deserializeJson(doc, http.getString());
	http.end();
	doc[0]["name"].as<String>().toCharArray(city_name, sizeof(city_name));
	strcpy(display_name, city_name); // copy city_name to display_city_name (for display purpose
	// remove the word "City" in the city name to ensure the title fits the screen
	char *pos = strstr(display_name, " City"); // copy city_name to display_city_name (for display purpose)
	if (pos != nullptr)
	{
		memmove(pos, pos + strlen(" City"), strlen(pos + strlen(" City")) + 1);
	}
	Serial.println("City name: " + String(display_name)); // after removing the word "City"
	// create title sprite and load font
	title_Sprite.setColorDepth(16);
	title_Sprite.createSprite(240, 11);
	title_Sprite.loadFont("Calibri-Bold-11", FFat);

	// create current weather sprite and load font
	current_weather_Sprite.setColorDepth(16);
	current_weather_Sprite.createSprite(160, 180);
	current_weather_Sprite.loadFont("Calibri-Bold-11", FFat);

	GetCurrentWeather_Ticker.attach(3600, startGetCurrentWeather);			  // trigger Get Current Weather every 1 hour
	GetForeCastWeather_Ticker.attach(24 * 3600 * 7, startGetForecastWeather); // trigger Get Forecast Weather every 7 days

	// wifi handle task
	xTaskCreatePinnedToCore(HandleWiFi, "WiFi_Handle", 3072, NULL, 1, &WiFi_Handle, 0);
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

	Serial.println("Finished setup");
	tft.println("Finished setup");
	delay(500);
	tft.fillScreen(TFT_BLACK); // clear display
	tft.unloadFont();

	xEventGroupSetBits(GetData_EventGroup, START_GET_CURRENT_WEATHER_FLAG | START_GET_AQI_FLAG | START_GET_UV_FLAG); // first run
}

void loop()
{
	// put your main code here, to run repeatedly:
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

void getCoordinates()
{
	// Define variables for averaging
	/* 	int numReadings = 5; // Number of readings to take
		double latSum = 0.0; // Sum of latitude readings
		double lonSum = 0.0; // Sum of longitude readings

		// Take multiple GPS readings and average them
		for (int i = 0; i < numReadings; i++)
		{
			// Get coordinates from GPS
			gps.encode(Serial1.read());

			// Add latitude and longitude readings to sum
			latSum += gps.location.lat();
			lonSum += gps.location.lng();

			// Wait for a short time between readings
			delay(100);
		}

		// Calculate average latitude and longitude
		lat = latSum / numReadings;
		lon = lonSum / numReadings; */
	lat = 10.894557;
	lon = 106.751430;       
	// London
	// lat = 51.507351;
	// lon = -0.127758;
}
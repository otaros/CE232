/*-----------------------------------Includes-----------------------------------*/
#include <Arduino.h>
#include <WiFi.h>
#include <Ticker.h>
#include <TFT_eSPI.h>
#include <freertos/FreeRTOS.h>

#include "GetData.h"
#include "VariableDeclaration.h"
#include "WiFiHandle.h"

using namespace fs;

char city_name[50];
char ssid[56], pass[56];

TinyGPSPlus gps;

TaskHandle_t WiFi_Handle;
TaskHandle_t GetCurrentWeather_Handle;
TaskHandle_t ProcessingCurrentWeather_Handle;
TaskHandle_t DisplayData_Handle;
TaskHandle_t GetForecastWeather_Handle;
TaskHandle_t ProcessingForecastWeather_Handle;
Ticker GetCurrentWeather_Ticker;
Ticker GetForeCastWeather_Ticker;

EventGroupHandle_t WiFi_EventGroup = xEventGroupCreate();
EventGroupHandle_t GetWeather_EventGroup = xEventGroupCreate();

QueueHandle_t current_weather_queue = xQueueCreate(1, sizeof(weather_data));
QueueHandle_t forecast_queue = xQueueCreate(8, sizeof(forecast));

TFT_eSPI tft = TFT_eSPI(320, 240);
TFT_eSprite title = TFT_eSprite(&tft);

HTTPClient http;

/*---------------------------------Prototypes-----------------------------------*/
void startGetCurrentWeather();
void startGetForecastWeather();
void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("System starting...");
  Serial1.begin(9600);
  Serial1.setPins(3, 4); // pin for GPS module

  if (!FFat.begin(false))
  {
    Serial.println("FFat Mount Failed");
    ESP.restart();
    return;
  }

  connectToWiFi(); // use for get WiFi information
  Serial.println("WiFi connected");

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer); // sync time with the internet
  Serial.println("Time synced");

  // tft.init();
  // tft.fillScreen(TFT_BLACK);
  // tft.setRotation(1);
  // tft.loadFont("Calibri-Bold-11", FFat);

  // title.createSprite(160, 30);
  // title.loadFont("Calibri-Bold-11", FFat);

  GetCurrentWeather_Ticker.attach(3600, startGetCurrentWeather);
  GetForeCastWeather_Ticker.attach(24 * 3600 * 7, startGetForecastWeather);

  xTaskCreatePinnedToCore(HandleWiFi, "WiFi_Handle", 4096, NULL, 1, &WiFi_Handle, 0);
  xTaskCreatePinnedToCore(GetCurrentWeather, "GetCurrentWeather", 8192, NULL, 1, &GetCurrentWeather_Handle, 0);
  xTaskCreatePinnedToCore(ProcessingCurrentWeather, "ProcessingCurrentWeather", 4096, NULL, 0, &ProcessingCurrentWeather_Handle, 0);
  xTaskCreatePinnedToCore(GetForecastWeather, "GetForecastWeather", 5120, NULL, 1, &GetForecastWeather_Handle, 0);
  xTaskCreatePinnedToCore(ProcessingForecastWeather, "ProcessingForecastWeather", 5120, NULL, 0, &ProcessingForecastWeather_Handle, 0);

  xEventGroupSetBits(GetWeather_EventGroup, START_GET_CURRENT_WEATHER);
  Serial.println("Finished setup");
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
  xResult = xEventGroupSetBitsFromISR(GetWeather_EventGroup, START_GET_CURRENT_WEATHER, &xHigherPriorityTaskWoken);

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
  xResult = xEventGroupSetBitsFromISR(GetWeather_EventGroup, START_GET_FORECAST_WEATHER, &xHigherPriorityTaskWoken);

  if (xResult != pdFAIL)
  {
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
  }
}

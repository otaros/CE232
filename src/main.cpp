/*-----------------------------------Includes-----------------------------------*/
#include <Arduino.h>
#include <WiFi.h>
#include <Ticker.h>
#include <TFT_eSPI.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>

#include "GetData.h"
#include "VariableDeclaration.h"
#include "WiFiHandle.h"

using namespace fs;

char city_name[50];
char ssid[56], pass[56];

TaskHandle_t WiFi_Handle;
TaskHandle_t GetCurrentWeather_Handle;
TaskHandle_t ProcessingCurrentWeather_Handle;
TaskHandle_t DisplayData_Handle;
TaskHandle_t GetForecastWeather_Handle;
TaskHandle_t ProcessingForecastWeather_Handle;
Ticker GetCurrentWeather_Ticker;
Ticker GetForeCastWeather_Ticker;

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
  Serial1.setPins(3, 4); // pin for GPS module
  Serial1.begin(9600);

  if (!FFat.begin(true))
  {
    Serial.println("FFat Mount Failed");
    ESP.restart();
    return;
  }

  connectToWiFi(); // use for get WiFi information

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer); // sync time with the internet

  tft.init();
  tft.fillScreen(TFT_BLACK);
  tft.setRotation(1);
  tft.loadFont("Calibri-Bold-11", FFat);

  title.createSprite(160, 30);
  title.loadFont("Calibri-Bold-11", FFat);

  GetCurrentWeather_Ticker.attach(3600, startGetCurrentWeather);
  GetForeCastWeather_Ticker.attach(24 * 3600 * 7, startGetForecastWeather);

  char StackSize[8192];

  xTaskCreatePinnedToCore(HandleWiFi, "WiFi_Handle", sizeof(StackSize), NULL, 1, &WiFi_Handle, 0);
  xTaskCreatePinnedToCore(GetCurrentWeather, "GetCurrentWeather", sizeof(StackSize), NULL, 1, &GetCurrentWeather_Handle, 1);
  xTaskCreatePinnedToCore(ProcessingCurrentWeather, "ProcessingCurrentWeather", sizeof(StackSize), NULL, 0, &ProcessingCurrentWeather_Handle, 0);
  xTaskCreatePinnedToCore(GetForecastWeather, "GetForecastWeather", sizeof(StackSize), NULL, 1, &GetForecastWeather_Handle, 0);
  xTaskCreatePinnedToCore(ProcessingForecastWeather, "ProcessingForecastWeather", sizeof(StackSize), NULL, 0, &ProcessingForecastWeather_Handle, 0);
}

void loop()
{
  // put your main code here, to run repeatedly:
}

void startGetCurrentWeather()
{
  // xTaskNotify(GetCurrentWeather_Handle, 1, eSetValueWithOverwrite);
}

void startGetForecastWeather()
{
  // xTaskNotify(GetForecastWeather_Handle, 1, eSetValueWithOverwrite);
}

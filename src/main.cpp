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

char city_name[50];
char ssid[56], pass[56];

struct tm structTime;

TinyGPSPlus gps;

TaskHandle_t WiFi_Handle;
TaskHandle_t GetCurrentWeather_Handle;
TaskHandle_t ProcessingCurrentWeather_Handle;
TaskHandle_t GetForecastWeather_Handle;
TaskHandle_t ProcessingForecastWeather_Handle;
TaskHandle_t DisplayTitle_Handle;
TaskHandle_t DisplayCurrentWeather_Handle;
Ticker GetCurrentWeather_Ticker;
Ticker GetForeCastWeather_Ticker;

EventGroupHandle_t WiFi_EventGroup = xEventGroupCreate();
EventGroupHandle_t GetWeather_EventGroup = xEventGroupCreate();

SemaphoreHandle_t httpclient_mutex = xSemaphoreCreateMutex();

QueueHandle_t current_weather_queue = xQueueCreate(2, sizeof(weather_data));
QueueHandle_t forecast_queue = xQueueCreate(8, sizeof(forecast));

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite title_Sprite = TFT_eSprite(&tft);
TFT_eSprite current_weather_Sprite = TFT_eSprite(&tft);

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
  Serial1.setPins(4, 5); // pin for GPS module

  if (!psramFound())
  {
    Serial.println("Error: PSRAM not found!");
    while (true)
      ;
  }
  psramInit();

  tft.init();
  tft.setTextWrap(true, true);
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setRotation(1);
  tft.setCursor(0, 0);
  tft.println("System starting...");
  delay(500);

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

  Serial.println("Connecting to WiFi...");
  tft.println("Connecting to WiFi...");
  connectToWiFi(); // use for get WiFi information
  Serial.println("WiFi connected to " + String(ssid));
  tft.println("WiFi connected to " + String(ssid));
  delay(500);

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer); // sync time with the internet
  tft.print("Waiting for time sync...");
  Serial.print("Waiting for time sync...");
  while (!getLocalTime(&structTime))
  {
    Serial.print(".");
    tft.print(".");
    delay(500);
  }
  Serial.println("");
  tft.println("");
  Serial.println("Time synced");
  tft.println("Time synced");
  delay(500);

  // create title sprite and load font
  title_Sprite.createSprite(240, 11);
  title_Sprite.setColorDepth(16);
  title_Sprite.loadFont("Calibri-Bold-11", FFat);

  // create current weather sprite and load font
  current_weather_Sprite.createSprite(240, 100);
  current_weather_Sprite.setColorDepth(16);
  current_weather_Sprite.loadFont("Calibri-Bold-11", FFat);

  GetCurrentWeather_Ticker.attach(3600, startGetCurrentWeather);            // trigger Get Current Weather every 1 hour
  GetForeCastWeather_Ticker.attach(24 * 3600 * 7, startGetForecastWeather); // trigger Get Forecast Weather every 7 days

  xTaskCreatePinnedToCore(HandleWiFi, "WiFi_Handle", 3072, NULL, 1, &WiFi_Handle, 0);
  xTaskCreatePinnedToCore(GetCurrentWeather, "GetCurrentWeather", 8192, NULL, 1, &GetCurrentWeather_Handle, 0);
  xTaskCreatePinnedToCore(ProcessingCurrentWeather, "ProcessingCurrentWeather", 5120, NULL, 1, &ProcessingCurrentWeather_Handle, 0);
  xTaskCreatePinnedToCore(GetForecastWeather, "GetForecastWeather", 8192, NULL, 1, &GetForecastWeather_Handle, 0);
  xTaskCreatePinnedToCore(ProcessingForecastWeather, "ProcessingForecastWeather", 5120, NULL, 1, &ProcessingForecastWeather_Handle, 0);

  xTaskCreatePinnedToCore(DisplayTitle, "Display Title", 2048, NULL, 1, &DisplayTitle_Handle, 1);
  xTaskCreatePinnedToCore(DisplayCurrentWeather, "Display Current Weather", 4096, NULL, 1, &DisplayCurrentWeather_Handle, 1);

  xEventGroupSetBits(GetWeather_EventGroup, START_GET_CURRENT_WEATHER_FLAG); // first run

  Serial.println("Finished setup");
  tft.println("Finished setup");
  delay(500);
  tft.fillScreen(TFT_BLACK); // clear display
  tft.unloadFont();
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
  xResult = xEventGroupSetBitsFromISR(GetWeather_EventGroup, START_GET_CURRENT_WEATHER_FLAG, &xHigherPriorityTaskWoken);

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
  xResult = xEventGroupSetBitsFromISR(GetWeather_EventGroup, START_GET_FORECAST_WEATHER_FLAG, &xHigherPriorityTaskWoken);

  if (xResult != pdFAIL)
  {
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
  }
}

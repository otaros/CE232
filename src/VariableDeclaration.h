#pragma once

#ifndef __VARIABLEDECLARATION_H__
#define __VARIABLEDECLARATION_H__

#include <Arduino.h>
#include <freertos/event_groups.h>
#include <string>
#include "time.h"
#include <HTTPClient.h>
#include <WiFi.h>
#include <FS.h>
#include <FFat.h>

#include "JsonPSRAMAllocator.h"
#include <TFT_eSPI.h>
#include <TimeLib.h>

#include "Utilities.h"

#define KEY 21
#define UP_KEY 39
#define DOWN_KEY 40
#define LEFT_KEY 41
#define RIGHT_KEY 42

#define REQUEST_WIFI_FLAG (1 << 0)
#define WIFI_IS_AVAILABLE_FOR_USE_FLAG (1 << 1)
#define DONE_USING_WIFI_FLAG (1 << 2)

#define START_GET_CURRENT_WEATHER_FLAG (1 << 0)
#define DONE_GET_CURRENT_WEATHER_FLAG (1 << 1)
#define DONE_PROCESSING_CURRENT_WEATHER_FLAG (1 << 2)
#define START_GET_FORECAST_WEATHER_FLAG (1 << 3)
#define DONE_GET_FORECAST_WEATHER_FLAG (1 << 4)
#define DONE_PROCESSING_FORECAST_WEATHER_FLAG (1 << 5)
#define START_GET_AQI_FLAG (1 << 6)
#define DONE_GET_AQI_FLAG (1 << 7)
#define START_GET_UV_FLAG (1 << 8)
#define DONE_GET_UV_FLAG (1 << 9)

#define FORECAST_NUMS 8

#define NUMS_FLOW 2

#define NUMS_OPTION 5

#define MAIN (1 << 0)
#define MENU (1 << 1)
#define THREE_HOURS_FORECAST (1 << 2)
#define INPUT_API (1 << 3)
#define INPUT_WIFI_CREDENTIALS (1 << 4)

struct weather_data
{
    float temp, temp_min, temp_max, feels_like;
    uint16_t pressure;
    uint8_t humidity;
    float wind_speed;
    uint16_t wind_deg;
    long sunrise, sunset;
    char icon[4];
};

struct forecast
{
    char icon[4];
    ulong dt;
    float temp_max, temp_min;
    uint8_t humidity;
};

extern uint8_t aqi;
extern double uv;
extern char ssid[56], pass[56];
const char ntpServer[] = "time.google.com";
extern int gmtOffset_sec; // GMT +7
const int daylightOffset_sec = 0;

const char openweather_api_key[] = "bb26bc20fc2f36129e121e0a13e23c1a";
const char uv_api_key[] = "openuv-fltferlg7o9v4l-io";
const char timezone_api_key[] = "RPBJWVWYLAIJ";
const char aqi_api_key[] = "d6a711a78613e89f8257f210012b76e89d1557d8";
const char opencage_api_key[] = "28e7dd0fe11645a08093c3017ff06468";

extern HTTPClient http;

extern struct tm structTime;
// extern TinyGPSPlus gps;
extern char name[], display_name[];
extern double lat, lon;

extern TFT_eSPI tft;
extern TFT_eSprite title_Sprite;
extern TFT_eSprite current_weather_Sprite;
extern TFT_eSprite forecast_Sprite1, forecast_Sprite2, forecast_Sprite3;
extern TFT_eSprite Menu_Sprite;
extern TFT_eSprite menu_cursor_Sprite;

extern TaskHandle_t WorkingFlowControl_Handle;
extern TaskHandle_t WiFi_Handle;
extern TaskHandle_t GetCurrentWeather_Handle;
extern TaskHandle_t ProcessingCurrentWeather_Handle;
extern TaskHandle_t GetForecastWeather_Handle;
extern TaskHandle_t ProcessingForecastWeather_Handle;
extern TaskHandle_t GetAQI_Handle;
extern TaskHandle_t GetUV_Handle;
extern TaskHandle_t DisplayTitle_Handle;
extern TaskHandle_t DisplayCurrentWeather_Handle;
extern TaskHandle_t DisplayForecastWeather_Handle;
extern TaskHandle_t DisplayMenu_Handle;
extern TaskHandle_t MenuControl_Handle;
// extern TaskHandle_t ErrorMonitor_Handle;

extern QueueHandle_t current_weather_queue;
extern QueueHandle_t forecast_queue;

extern EventGroupHandle_t WiFi_EventGroup;
extern EventGroupHandle_t GetData_EventGroup;
extern EventGroupHandle_t WorkingFlow_EventGroup;
extern EventGroupHandle_t CurrentFlow_EventGroup;

extern SemaphoreHandle_t coordinate_mutex;
extern SemaphoreHandle_t http_mutex;

// function prototypes
void getLocation();
#endif // __VARIABLEDECLARATION_H__
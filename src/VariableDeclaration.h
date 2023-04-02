#pragma once

#ifndef __VARIABLEDECLARATION_H__
#define __VARIABLEDECLARATION_H__

#include <Arduino.h>
#include <string>
#include "time.h"
#include <TinyGPS++.h>
#include <TFT_eSPI.h>
#include <freertos/event_groups.h>

#include "JsonPSRAMAllocator.h"
#include <WiFi.h>
#include <FS.h>
#include <FFat.h>

#define REQUEST_WIFI_FLAG (1 << 0)
#define WIFI_IS_AVAILABLE_FOR_USE_FLAG (1 << 1)
#define DONE_USING_WIFI_FLAG (1 << 2)

#define START_GET_CURRENT_WEATHER_FLAG (1 << 0)
#define DONE_GET_CURRENT_WEATHER_FLAG (1 << 1)
#define DONE_PROCESSING_CURRENT_WEATHER_FLAG (1 << 2)
#define START_GET_FORECAST_WEATHER_FLAG (1 << 3)
#define DONE_GET_FORECAST_WEATHER_FLAG (1 << 4)
#define DONE_PROCESSING_FORECAST_WEATHER_FLAG (1 << 5)

#define FORECAST_NUMS 8

struct weather_data
{
    float temp, temp_min, temp_max, feels_like;
    float pressure, humidity;
    float wind_speed;
    uint16_t wind_deg;
    ulong sunrise, sunset;
    char icon[4];
};

struct forecast
{
    char icon[4];
    ulong dt;
    float temp_max, temp_min;
};

extern char ssid[56], pass[56];
const char ntpServer[] = "pool.ntp.org";
const long gmtOffset_sec = 25200; // GMT +7
const int daylightOffset_sec = 0; // UTC +7

const char api_key[] = "bb26bc20fc2f36129e121e0a13e23c1a";

extern struct tm structTime;
extern TinyGPSPlus gps;
extern char city_name[50];

extern TFT_eSPI tft;
extern TFT_eSprite title;

extern TaskHandle_t WiFi_Handle;
extern TaskHandle_t GetCurrentWeather_Handle;
extern TaskHandle_t ProcessingCurrentWeather_Handle;
extern TaskHandle_t GetForecastWeather_Handle;
extern TaskHandle_t ProcessingForecastWeather_Handle;
extern TaskHandle_t DisplayTitle_Handle;
extern TaskHandle_t DisplayCurrentWeather_Handle;

extern QueueHandle_t current_weather_queue;
extern QueueHandle_t forecast_queue;

extern EventGroupHandle_t WiFi_EventGroup;
extern EventGroupHandle_t GetWeather_EventGroup;

#endif // __VARIABLEDECLARATION_H__
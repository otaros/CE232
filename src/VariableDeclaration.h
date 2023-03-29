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

#define REQUEST_WIFI (1 << 0)
#define WIFI_IS_AVAILABLE_FOR_USE (1 << 1)
#define DONE_USING_WIFI (1 << 2)

#define START_GET_CURRENT_WEATHER (1 << 0)
#define DONE_GET_CURRENT_WEATHER (1 << 1)
#define START_PROCESSING_CURRENT_WEATHER (1 << 2)
#define START_GET_FORECAST_WEATHER (1 << 3)
#define DONE_GET_FORECAST_WEATHER (1 << 4)
#define START_PROCESSING_FORECAST_WEATHER (1 << 5)

extern char ssid[56], pass[56];
const char ntpServer[] = "pool.ntp.org";
const long gmtOffset_sec = 25200; // GMT +7
const int daylightOffset_sec = 0; // UTC +7

const char api_key[] = "bb26bc20fc2f36129e121e0a13e23c1a";

static struct tm structTime;
static TinyGPSPlus gps;
extern char city_name[50];

extern TFT_eSPI tft;
extern TFT_eSprite title;

extern TaskHandle_t WiFi_Handle;
extern TaskHandle_t GetCurrentWeather_Handle;
extern TaskHandle_t ProcessingCurrentWeather_Handle;
extern TaskHandle_t DisplayData_Handle;
extern TaskHandle_t GetForecastWeather_Handle;
extern TaskHandle_t ProcessingForecastWeather_Handle;

extern QueueHandle_t current_weather_queue;
extern QueueHandle_t forecast_queue;

extern EventGroupHandle_t WiFi_EventGroup;
extern EventGroupHandle_t GetWeather_EventGroup;

#endif // __VARIABLEDECLARATION_H__
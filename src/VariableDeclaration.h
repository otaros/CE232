#pragma once

#ifndef __VARIABLEDECLARATION_H__
#define __VARIABLEDECLARATION_H__

#include <Arduino.h>
#include <string>
#include "time.h"
#include <TinyGPS++.h>
#include <TFT_eSPI.h>

#include "JsonPSRAMAllocator.h"
#include <WiFi.h>
#include <FS.h>
#include <FFat.h>

extern char ssid[56], pass[56];
const char ntpServer[] = "pool.ntp.org";
const long gmtOffset_sec = 25200; // GMT +7
const int daylightOffset_sec = 0; // UTC +7

const String api_key = "bb26bc20fc2f36129e121e0a13e23c1a";

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

#endif // __VARIABLEDECLARATION_H__
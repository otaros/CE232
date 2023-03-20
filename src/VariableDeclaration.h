#pragma once

#include <Arduino.h>
#include <string>
#include "time.h"
#include <TinyGPS++.h>
#include <TFT_eSPI.h>

// const char ssid[] = "DESKTOP-OI97DKV 6851";
// const char password[] = "40n9X+82";
const char ntpServer[] = "pool.ntp.org";
const long gmtOffset_sec = 25200; // GMT +7
const int daylightOffset_sec = 0; // UTC +7

const String api_key = "bb26bc20fc2f36129e121e0a13e23c1a";

static struct tm structTime;
static TinyGPSPlus gps;

static TFT_eSPI tft = TFT_eSPI();
static TFT_eSprite current_weather = TFT_eSprite(&tft);

extern TaskHandle_t GetData_Handle;
extern TaskHandle_t ProcessingData_Handle;
extern TaskHandle_t DisplayData_Handle;
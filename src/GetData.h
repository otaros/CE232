#pragma once

#ifndef __GETDATA_H__
#define __GETDATA_H__

#include <FreeRTOS.h>
#include <Arduino.h>
#include <string>
#include <HTTPClient.h>
#include <TimeLib.h>
#include <FS.h>
#include <FFat.h>

#include "VariableDeclaration.h"

struct weather_data
{
    float temp, temp_min, temp_max, feels_like;
    float pressure, humidity;
    float wind_speed;
    uint16_t wind_deg;
    ulong sunrise, sunset;
    char icon[3];
};

struct forecast
{
    char icon[3];
    ulong dt;
    float temp_max, temp_min;
};
extern HTTPClient http;
static double lat, lon;
static uint16_t http_code;


void GetCurrentWeather(void *pvParameters);
void ProcessingCurrentWeather(void *pvParameters);
void GetForecastWeather(void *pvParameters);
void ProcessingForecastWeather(void *pvParameters);
void getCoordinates(double *, double *);

#endif // __GETDATA_H__
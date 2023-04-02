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

extern HTTPClient http;
static double lat, lon;
static uint16_t http_code;


void GetCurrentWeather(void *pvParameters);
void ProcessingCurrentWeather(void *pvParameters);
void GetForecastWeather(void *pvParameters);
void ProcessingForecastWeather(void *pvParameters);
void getCoordinates(double *, double *);

#endif // __GETDATA_H__
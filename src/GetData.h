#pragma once

#ifndef __GETDATA_H__
#define __GETDATA_H__

#include <FreeRTOS.h>
#include <Arduino.h>
#include <string>
#include <TimeLib.h>
#include <FS.h>
#include <FFat.h>

#include "VariableDeclaration.h"



void GetCurrentWeather(void *pvParameters);
void ProcessingCurrentWeather(void *pvParameters);
void GetForecastWeather(void *pvParameters);
void ProcessingForecastWeather(void *pvParameters);
void GetAQI(void *pvParameters);
void GetUV(void *pvParameters);

#endif // __GETDATA_H__
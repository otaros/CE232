#pragma once

#ifndef __GETDATA_H__
#define __GETDATA_H__

#include <FreeRTOS.h>
#include <Arduino.h>
#include <string>
#include <HTTPClient.h>
#include <JsonPSRAMAllocator.h>
#include <FS.h>
#include <FFat.h>

#include "VariableDeclaration.h"

static HTTPClient http;
static double lat, lon;
static uint16_t http_code;
static String city_name;

void GetData(void *pvParameters);
void ProcessingData(void *pvParameters);
void getCoordinates(double *, double *);

#endif // __GETDATA_H__
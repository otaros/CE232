#pragma one
#include "VariableDeclaration.h"

#ifndef __DISPLAY_H__
#define __DISPLAY_H__

static char title_text[50];
static char date[40];

uint16_t getColorUV(double uv);
uint16_t getAQIColor(uint8_t aqi);

void DisplayTitle(void *pvParameters);
void DisplayCurrentWeather(void *pvParameters);
// void DisplayForecastWeather(void *pvParameters);
void DisplayThreeHoursForecast(void *pvParameters);
void displayMenu();

#endif // __DISPLAY_H__
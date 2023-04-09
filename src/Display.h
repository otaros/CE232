#pragma one
#include "VariableDeclaration.h"

#ifndef __DISPLAY_H__
#define __DISPLAY_H__

static char title_text[50];
static char date[40];

uint16_t getColorUV(double uv);
uint16_t read16(fs::File &f);
uint32_t read32(fs::File &f);

void DisplayTitle(void *pvParameters);
void DisplayCurrentWeather(void *pvParameters);
void drawBmp(const char *filename, int16_t x, int16_t y);
void drawBmpToSprite(const char *filename, int16_t x, int16_t y, TFT_eSprite *spr);

#endif // __DISPLAY_H__
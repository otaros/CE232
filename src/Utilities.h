#pragma once

#ifndef __UTILITIES_H__
#define __UTILITIES_H__

#include <Arduino.h>

#include "VariableDeclaration.h"

enum BUTTON_STATE
{
    KEY_PRESSED,
    KEY_HOLD,
    UP,
    DOWN,
    LEFT,
    RIGHT,
    NONE,
};

uint16_t read16(fs::File &f);
uint32_t read32(fs::File &f);
BUTTON_STATE getButtonState();
String degToCompass(uint16_t num);
void drawBmp(const char *filename, int16_t x, int16_t y);
void drawBmpToSprite(const char *filename, int16_t x, int16_t y, TFT_eSprite *spr);

#endif // !__UTILITIES_H__
#pragma once

#ifndef __MENU_H__
#define __MENU_H__

#include <VariableDeclaration.h>
#include <WiFiHandle.h>

extern int8_t menu_index;

void MenuControl(void *pvParameters);
void ButtonMonitoring(void *pvParameters);

#endif // __MENU_H__
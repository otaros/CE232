#pragma once

#ifndef __WIFIHANDLE_H__
#define __WIFIHANDLE_H__

#include <FS.h>
#include <FFat.h>

#include "VariableDeclaration.h"

void connectToWiFi();
void HandleWiFi(void *pvParameters);

#endif // __WIFIHANDLE_H__
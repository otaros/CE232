#pragma once

#ifndef __WIFIHANDLE_H__
#define __WIFIHANDLE_H__

#include <FS.h>
#include <FFat.h>
#include <WebServer.h>

#include "VariableDeclaration.h"

extern WebServer server;

void connectToWiFi();
void HandleWiFi(void *pvParameters);
void createwebserver();
void handleRoot();
void openwebserver();


#endif // __WIFIHANDLE_H__
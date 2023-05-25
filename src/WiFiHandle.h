#pragma once

#ifndef __WIFIHANDLE_H__
#define __WIFIHANDLE_H__

#include <FS.h>
#include <FFat.h>
#include <WebServer.h>

#include "VariableDeclaration.h"

extern WebServer server;

void connectToWiFi();
void openWebSever_WiFiCredentials(const char *ssid, const char *pass);
void HandleWiFi(void *pvParameters);
void handleWiFiInput();
void handleWiFiSave();
void handleLocationInput();
void handleLocationSave();
void inputLocation(const char *ssid, const char *pass);
#endif // __WIFIHANDLE_H__
/*-----------------------------------Includes-----------------------------------*/
#include <Arduino.h>
#include <WiFi.h>
#include <Ticker.h>
#include <TFT_eSPI.h>
#include <freertos/FreeRTOS.h>

#include "GetData.h"
#include "VariableDeclaration.h"

using namespace fs;

TaskHandle_t GetData_Handle;
TaskHandle_t ProcessingData_Handle;
TaskHandle_t DisplayData_Handle;
Ticker GetDataTicker;

void startGetData()
{
  xTaskNotify(GetData_Handle, 1, eSetValueWithOverwrite);
}

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial1.setPins(3, 4);
  Serial1.begin(9600);
  if (!FFat.begin(true))
  {
    Serial.println("FFat Mount Failed");
    ESP.restart();
    return;
  }
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  delay(100);
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer); // sync time with the internet
  tft.init();
  tft.setRotation(1);
  tft.loadFont("YuGothicUI-Bold-11", FFat);
  GetDataTicker.attach(3600, startGetData);
  xTaskCreatePinnedToCore(GetData, "GetData", 10000, NULL, 1, &GetData_Handle, 0);
  xTaskCreatePinnedToCore(ProcessingData, "ProcessingData", 10000, NULL, 1, &ProcessingData_Handle, 0);
}

void loop()
{
  // put your main code here, to run repeatedly:
}
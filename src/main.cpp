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

/*---------------------------------Prototypes-----------------------------------*/
void startGetData();
void connectToWiFi();
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

  connectToWiFi();
  delay(100);

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer); // sync time with the internet

  tft.init();
  tft.fillScreen(TFT_BLACK);
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

void startGetData()
{
  xTaskNotify(GetData_Handle, 1, eSetValueWithOverwrite);
}

void connectToWiFi()
{
  bool found = false; // bool to check if the wifi credentials are stored in the file
  String ssid, pass;
  WiFi.mode(WIFI_STA);
  File data = FFat.open("/wifi_credentials.json", "r"); // read data from file
  PSRAMJsonDocument doc(4096);
  deserializeJson(doc, data.readString());
  data.close();
  int n = WiFi.scanNetworks();
  for (int i = 0; i < n; i++) // check if the wifi credentials are stored in the file
  {
    pass = doc[WiFi.SSID(i)].as<String>();
    if (pass != "null")
    {
      WiFi.begin(WiFi.SSID(i).c_str(), pass.c_str());
      ulong point = millis();
      while (WiFi.status() != WL_CONNECTED)
      {
        if (millis() - point > 5000)
        {
          goto failed_connection; // if the wifi credentials are wrong, get new ones
        }
        delay(1000);
        found = true;
        Serial.println("Connecting to WiFi...");
      }
      return;
    }
  }
  if (found == false) // if the wifi credentials are not stored in the file
  {
  // get ssid and pass from user
  failed_connection:
    Serial.print("Please input your SSID: ");
    // ssid = Serial.readStringUntil(10); // read until new line
    Serial.print("Please input your password: ");
    // pass = Serial.readStringUntil(10); // read until new line
    WiFi.begin(ssid.c_str(), pass.c_str());
    ulong point = millis();
    while (WiFi.status() != WL_CONNECTED)
    {
      if (millis() - point > 5000)
      {
        goto failed_connection; // wrong password
      }
      delay(1000);
      Serial.println("Connecting to WiFi...");
    }
    data = FFat.open("/wifi_credentials.json", "w");
    doc[ssid] = pass;
    data.print(doc.as<String>());
  }
  return;
}

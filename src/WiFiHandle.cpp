#include "WiFiHandle.h"

void connectToWiFi()
{
    bool found = false; // bool to check if the wifi credentials are stored in the file
    WiFi.mode(WIFI_STA);
    File data = FFat.open("/wifi_credentials.json", "r"); // read data from file
    PSRAMJsonDocument doc(4096);
    deserializeJson(doc, data.readString());
    data.close();
    int n = WiFi.scanNetworks();
    while (WiFi.scanComplete() == WIFI_SCAN_RUNNING)
    {
        delay(100);
    }
    for (int i = 0; i < n; i++) // check if the wifi credentials are stored in the file
    {
        if (doc.containsKey(WiFi.SSID(i).c_str()))
        {
            doc[WiFi.SSID(i)].as<String>().toCharArray(pass, sizeof(pass));
            WiFi.begin(WiFi.SSID(i).c_str(), pass);
            ulong startAttemptTime = millis();
            while (WiFi.status() != WL_CONNECTED)
            {
                if (millis() - startAttemptTime > 5000)
                {
                    break;
                }
                delay(1000);
            }
            if (WiFi.status() == WL_CONNECTED)
            {
                found = true;
                WiFi.SSID(i).toCharArray(ssid, sizeof(ssid));

                break;
            }
        }
    }
    if (!found) // if the wifi credentials are not stored in the file
    {
        // get ssid and pass from user
        while (true)
        {
            Serial.print("Please input your SSID: ");
            Serial.readStringUntil('\n').toCharArray(ssid, sizeof(ssid));
            Serial.print("Please input your password: ");
            Serial.readStringUntil('\n').toCharArray(pass, sizeof(pass));
            WiFi.begin(ssid, pass);
            unsigned long startAttemptTime = millis();
            while (WiFi.status() != WL_CONNECTED)
            {
                if (millis() - startAttemptTime > 5000)
                {
                    break;
                }
                delay(1000);
            }
            if (WiFi.status() == WL_CONNECTED)
            {
                data = FFat.open("/wifi_credentials.json", "w");
                doc[ssid] = pass;
                data.print(doc.as<String>());
                data.flush();
                data.close();
                return;
            }
        }
    }
}

void HandleWiFi(void *pvParameters)
{
    uint32_t *notificationValue = (uint32_t *)ps_malloc(sizeof(uint32_t));
    TaskHandle_t taskHandle;
    for (;;)
    {
        // xTaskNotifyWait(0, 0xFFFFFFFF, notificationValue, portMAX_DELAY);
        taskHandle = xTaskGetCurrentTaskHandle(); // get the handle of the current task
        if (*notificationValue == 1)
        {
            if (WiFi.status() != WL_CONNECTED)
            {
                WiFi.mode(WIFI_STA);
                WiFi.begin(ssid, pass);
                unsigned long startAttemptTime = millis();
                while (WiFi.status() != WL_CONNECTED)
                {
                    if (millis() - startAttemptTime > 1000)
                    {
                        break;
                    }
                    delay(100);
                }
            }
            if (WiFi.status() != WL_CONNECTED) // if wifi still not connected
            {
                connectToWiFi();
            }
            if (WiFi.status() == WL_CONNECTED)
            {
            }
            // xTaskNotifyWait(0, 0xFFFFFFFF, notificationValue, portMAX_DELAY);
            if (*notificationValue == 2)
            {
                WiFi.disconnect();
                WiFi.mode(WIFI_OFF);
            }
        }
    }
}

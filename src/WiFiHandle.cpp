#include "WiFiHandle.h"

WebServer server(80);

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
            openwebserver();
            Serial.println("Please connect to WiFi: Weather Station");
            Serial.println("And connect to this IP: " + WiFi.softAPIP().toString());
            while (WiFi.status() != WL_CONNECTED)
            {
                delay(500);
                server.handleClient();
                if (WiFi.status() == WL_CONNECTED)
                {
                    server.stop();
                    WiFi.softAPdisconnect(true);
                    WiFi.mode(WIFI_STA);
                    break;
                }
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
    for (;;)
    {
        xEventGroupWaitBits(WiFi_EventGroup, REQUEST_WIFI, pdTRUE, pdFALSE, portMAX_DELAY);
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
            xEventGroupSetBits(WiFi_EventGroup, WIFI_IS_AVAILABLE_FOR_USE);
        }
        // xTaskNotifyWait(0, 0xFFFFFFFF, notificationValue, portMAX_DELAY);
        xEventGroupWaitBits(WiFi_EventGroup, DONE_USING_WIFI, pdTRUE, pdFALSE, portMAX_DELAY);
        WiFi.disconnect();
        WiFi.mode(WIFI_OFF);
    }
}

void openwebserver()
{
    WiFi.disconnect();
    WiFi.mode(WIFI_AP_STA);

    WiFi.softAP("Weather Station ");

    createwebserver();
    server.begin();
}

void handleRoot()
{
    File file = FFat.open("/index.html", "r"); // Open the file
    String html = file.readString();           // Read the file
    file.close();                              // Close the file
    server.send(200, "text/html", html);
}

void handleSave()
{
    server.arg("ssid").toCharArray(ssid, sizeof(ssid));
    server.arg("pass").toCharArray(pass, sizeof(pass));
    File file = FFat.open("/save.html", "r");
    String html = file.readString();
    file.close();
    server.send(200, "text/html", html);
    WiFi.begin(ssid, pass);
}

void createwebserver()
{
    server.on("/", handleRoot);
    server.on("/submited", handleSave);
}
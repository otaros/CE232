#include "GetData.h"

void GetData(void *pvParameters)
{
    uint32_t notificationValue;
    for (;;)
    {
        // get location phase
        xTaskNotifyWait(0x00, 0xFF, NULL, portMAX_DELAY);
        getCoordinates(&lat, &lon);
        getLocalTime(&structTime);
        String location = "http://api.openweathermap.org/geo/1.0/reverse?lat=" + String(lat) + "&lon=" + String(lon) + "&limit=1&appid=" + api_key;
        http.begin(location);
        http_code = http.GET();
        String payload = http.getString();
        PSRAMJsonDocument doc(4096);
        deserializeJson(doc, payload);
        delete &location; // Free memory
        String name = doc[0]["name"];
        city_name = name;
        http.end();
        // get data phase
        String weather = "http://api.openweathermap.org/data/2.5/weather?q=" + city_name + "&appid=" + api_key + "&units=metric";
        http.begin(weather);
        http_code = http.GET();
        payload = http.getString();
        deserializeJson(doc, payload);
        File data = FFat.open("/data.txt", "w", true);
        data.print(payload);
        data.close();
        delete &weather;          // Free memory
        doc.~PSRAMJsonDocument(); // Free memory
        http.end();
        xTaskNotify(ProcessingData_Handle, 1, eSetValueWithOverwrite);
    }
}
void ProcessingData(void *pvParameters)
{
    uint32_t *notificationValue = (uint32_t *)ps_malloc(sizeof(uint32_t));
    for (;;)
    {
        xTaskNotifyWait(0, 0xFFFFFFFF, notificationValue, portMAX_DELAY);
        File data = FFat.open("/data.txt", "r");
        String payload = data.readString();
        PSRAMJsonDocument doc(4096);
        deserializeJson(doc, payload);
    }
}
void getCoordinates(double *lat, double *lon)
{
    // Get coordinates from GPS
    gps.encode(Serial1.read());
    *lat = gps.location.lat();
    *lon = gps.location.lng();
}
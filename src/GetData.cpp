#include "GetData.h"

void GetCurrentWeather(void *pvParameters)
{
    char location[128];
    char weather[128];
    char payload[640];
    uint32_t *notificationValue = (uint32_t *)ps_malloc(sizeof(uint32_t));
    uint32_t *WiFistateValue = (uint32_t *)ps_malloc(sizeof(uint32_t));
    PSRAMJsonDocument doc(1024);
    for (;;)
    {
        // get location phase
        // xTaskNotifyWait(0x00, 0xFFFFFFFF, notificationValue, portMAX_DELAY);
        // xTaskNotify(WiFi_Handle, 1, eSetValueWithOverwrite);
        // xTaskNotifyWait(0x00, 0xFFFFFFFF, WiFistateValue, portMAX_DELAY);
        getCoordinates(&lat, &lon);
        getLocalTime(&structTime);
        snprintf(location, sizeof(location), "http://api.openweathermap.org/geo/1.0/reverse?lat=%.6f&lon=%.6f&limit=1&appid=%s", lat, lon, api_key);
        http.begin(location);
        http_code = http.GET();
        http.getString().toCharArray(payload, sizeof(payload));
        deserializeJson(doc, payload);
        doc[0]["name"].as<String>().toCharArray(city_name, sizeof(city_name));
        http.end();
        // get data phase
        snprintf(weather, sizeof(weather), "http://api.openweathermap.org/data/2.5/weather?q=%s&appid=%s&units=metric", city_name, api_key);
        http.begin(weather);
        http_code = http.GET();
        http.getString().toCharArray(payload, sizeof(payload));
        deserializeJson(doc, payload);
        File data = FFat.open("/data.txt", "w", true);
        data.print(payload);
        data.flush();
        data.close();
        http.end();
        // xTaskNotify(WiFi_Handle, 2, eSetValueWithOverwrite);
        // xTaskNotify(ProcessingCurrentWeather_Handle, 1, eSetValueWithOverwrite);
    }
}
void ProcessingCurrentWeather(void *pvParameters)
{
    uint32_t *notificationValue = (uint32_t *)ps_malloc(sizeof(uint32_t));
    for (;;)
    {
        // xTaskNotifyWait(0, 0xFFFFFFFF, notificationValue, portMAX_DELAY);
        // Processing current weather
        File data = FFat.open("/data.txt", "r");
        String payload = data.readString();
        PSRAMJsonDocument doc(4096);
        deserializeJson(doc, payload);
        weather_data current;
        current.temp = doc["main"]["temp"];
        current.temp_min = doc["main"]["temp_min"];
        current.temp_max = doc["main"]["temp_max"];
        current.feels_like = doc["main"]["feels_like"];
        current.pressure = doc["main"]["pressure"];
        current.humidity = doc["main"]["humidity"];
        current.wind_speed = doc["wind"]["speed"];
        current.wind_deg = doc["wind"]["deg"];
        current.sunrise = doc["sys"]["sunrise"];
        doc["weather"][0]["icon"].as<String>().toCharArray(current.icon, 3);
        data.close();
        xQueueSend(current_weather_queue, &current, portMAX_DELAY);
    }
}
void GetForecastWeather(void *pvParameters) // get 8-day forecast
{
    uint32_t *notificationValue = (uint32_t *)ps_malloc(sizeof(uint32_t));
    char forecast[128];
    char payload[4096];
    PSRAMJsonDocument doc(5120);
    // xTaskNotifyWait(0x00, 0xFFFFFFFF, notificationValue, portMAX_DELAY);
    for (;;)
    {
        // get data phase
        getCoordinates(&lat, &lon);
        snprintf(forecast, sizeof(forecast), "http://api.openweathermap.org/data/2.5/onecall?lat=%.6f&lon=%.6f&exclude=current,minutely,hourly,alerts&appid=%s&units=metric", lat, lon, api_key);
        http.begin(forecast);
        http_code = http.GET();
        http.getString().toCharArray(payload, sizeof(payload));
        deserializeJson(doc, payload);
        File data = FFat.open("/forecast.txt", "w", true);
        data.print(payload);
        data.flush();
        data.close();
        http.end();
        // xTaskNotify(ProcessingForecastWeather_Handle, 1, eSetValueWithOverwrite);
    }
}
void ProcessingForecastWeather(void *pvParameters)
{
    uint32_t *notificationValue = (uint32_t *)ps_malloc(sizeof(uint32_t));
    for (;;)
    {
        // xTaskNotifyWait(0, 0xFFFFFFFF, notificationValue, portMAX_DELAY);
        File data = FFat.open("/forecast.txt", "r");
    }
}
void getCoordinates(double *lat, double *lon)
{
    // Get coordinates from GPS
    gps.encode(Serial1.read());
    *lat = gps.location.lat();
    *lon = gps.location.lng();
}
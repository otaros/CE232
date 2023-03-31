#include "GetData.h"

void GetCurrentWeather(void *pvParameters)
{
    char location[128];
    char weather[128];
    char payload[640];
    PSRAMJsonDocument doc(1024);
    for (;;)
    {
        // get location phase
        xEventGroupWaitBits(GetWeather_EventGroup, START_GET_CURRENT_WEATHER, pdTRUE, pdTRUE, portMAX_DELAY);
        xEventGroupSetBits(WiFi_EventGroup, REQUEST_WIFI);
        Serial.println("Requesting wifi connection (Current Weather)");
        xEventGroupWaitBits(WiFi_EventGroup, WIFI_IS_AVAILABLE_FOR_USE, pdTRUE, pdTRUE, portMAX_DELAY);

        getCoordinates(&lat, &lon);
        snprintf(location, sizeof(location), "http://api.openweathermap.org/geo/1.0/reverse?lat=%.6f&lon=%.6f&limit=1&appid=%s", lat, lon, api_key);
        Serial.println(location); // for checking purpose
        http.begin(location);
        http_code = http.GET();
        http.getString().toCharArray(payload, sizeof(payload));
        http.end();
        deserializeJson(doc, payload);
        doc[0]["name"].as<String>().toCharArray(city_name, sizeof(city_name));
        // get data phase
        snprintf(weather, sizeof(weather), "http://api.openweathermap.org/data/2.5/weather?q=%s&appid=%s&units=metric", city_name, api_key);
        Serial.println(weather); // for checking purpose
        http.begin(weather);
        http_code = http.GET();
        http.getString().toCharArray(payload, sizeof(payload));
        http.end();
        Serial.println(payload); // show the payload
        deserializeJson(doc, payload);
        File data = FFat.open("/current.txt", "w", true);
        data.print(payload);
        data.flush();
        data.close();

        Serial.println("Done getting current weather");
        xEventGroupSetBits(WiFi_EventGroup, DONE_USING_WIFI);
        xEventGroupSetBits(GetWeather_EventGroup, DONE_GET_CURRENT_WEATHER);
        taskYIELD();
    }
}
void ProcessingCurrentWeather(void *pvParameters)
{
    for (;;)
    {
        // Processing current weather
        xEventGroupWaitBits(GetWeather_EventGroup, START_PROCESSING_CURRENT_WEATHER, pdTRUE, pdTRUE, portMAX_DELAY);
        File data = FFat.open("/current.txt", "r");
        String payload = data.readString();
        PSRAMJsonDocument doc(4096);
        deserializeJson(doc, payload);
        weather_data *current = (weather_data *)ps_malloc(sizeof(weather_data));
        current->temp = doc["main"]["temp"];
        current->temp_min = doc["main"]["temp_min"];
        current->temp_max = doc["main"]["temp_max"];
        current->feels_like = doc["main"]["feels_like"];
        current->pressure = doc["main"]["pressure"];
        current->humidity = doc["main"]["humidity"];
        current->wind_speed = doc["wind"]["speed"];
        current->wind_deg = doc["wind"]["deg"];
        current->sunrise = doc["sys"]["sunrise"];
        doc["weather"][0]["icon"].as<String>().toCharArray(current->icon, 3);
        data.close();

        xQueueSend(current_weather_queue, current, portMAX_DELAY);
        taskYIELD();
    }
}
void GetForecastWeather(void *pvParameters) // get 8-day forecast
{
    char forecast[128];
    char payload[4096];
    PSRAMJsonDocument doc(5120);
    for (;;)
    {
        // get data phase
        xEventGroupWaitBits(GetWeather_EventGroup, START_GET_FORECAST_WEATHER, pdTRUE, pdTRUE, portMAX_DELAY);
        xEventGroupSetBits(WiFi_EventGroup, REQUEST_WIFI);
        Serial.println("Requesting wifi connection (Forecast Weather))");
        xEventGroupWaitBits(WiFi_EventGroup, WIFI_IS_AVAILABLE_FOR_USE, pdTRUE, pdTRUE, portMAX_DELAY);

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

        Serial.println("Done getting forecast weather");
        xEventGroupSetBits(WiFi_EventGroup, DONE_USING_WIFI);
        xEventGroupSetBits(GetWeather_EventGroup, DONE_GET_FORECAST_WEATHER);
        taskYIELD();
    }
}
void ProcessingForecastWeather(void *pvParameters)
{
    for (;;)
    {
        xEventGroupWaitBits(GetWeather_EventGroup, DONE_GET_FORECAST_WEATHER, pdTRUE, pdTRUE, portMAX_DELAY);
        File data = FFat.open("/forecast.txt", "w", true);
    }
}
void getCoordinates(double *lat, double *lon)
{
    // Get coordinates from GPS
    // gps.encode(Serial1.read());
    // *lat = gps.location.lat();
    // *lon = gps.location.lng();
    *lat = 10.894557;
    *lon = 106.751430;
}
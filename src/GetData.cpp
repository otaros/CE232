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
        xEventGroupWaitBits(GetWeather_EventGroup, START_GET_CURRENT_WEATHER_FLAG, pdTRUE, pdTRUE, portMAX_DELAY);
        Serial.println("Start getting current weather");
        xEventGroupSetBits(WiFi_EventGroup, REQUEST_WIFI_FLAG);
        Serial.println("Requesting wifi connection (Current Weather)");
        xEventGroupWaitBits(WiFi_EventGroup, WIFI_IS_AVAILABLE_FOR_USE_FLAG, pdTRUE, pdTRUE, portMAX_DELAY);

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
        File data = FFat.open("/current.json", "w", true);
        data.print(doc.as<String>());
        data.flush();
        data.close();

        Serial.println("Done getting current weather");
        xEventGroupSetBits(WiFi_EventGroup, DONE_USING_WIFI_FLAG);
        xEventGroupSetBits(GetWeather_EventGroup, DONE_GET_CURRENT_WEATHER_FLAG);
        taskYIELD();
    }
}
void ProcessingCurrentWeather(void *pvParameters)
{
    for (;;)
    {
        // Processing current weather
        xEventGroupWaitBits(GetWeather_EventGroup, DONE_GET_CURRENT_WEATHER_FLAG, pdTRUE, pdTRUE, portMAX_DELAY);
        Serial.println("Start processing current weather");
        File data = FFat.open("/current.json", "r");
        PSRAMJsonDocument doc(4096);
        deserializeJson(doc, data.readString());
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
        doc["weather"][0]["icon"].as<String>().toCharArray(current->icon, 4);
        data.close();

        BaseType_t ret = xQueueSend(current_weather_queue, current, portMAX_DELAY);
        while (ret != pdTRUE)
        {
            ret = xQueueSend(current_weather_queue, current, portMAX_DELAY);
        }
        Serial.println("Done processing current weather");
        xEventGroupSetBits(GetWeather_EventGroup, DONE_PROCESSING_CURRENT_WEATHER_FLAG);
        taskYIELD(); // finish the task
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
        xEventGroupWaitBits(GetWeather_EventGroup, START_GET_FORECAST_WEATHER_FLAG, pdTRUE, pdTRUE, portMAX_DELAY);
        Serial.println("Start getting forecast weather");
        xEventGroupSetBits(WiFi_EventGroup, REQUEST_WIFI_FLAG);
        Serial.println("Requesting wifi connection (Forecast Weather))");
        xEventGroupWaitBits(WiFi_EventGroup, WIFI_IS_AVAILABLE_FOR_USE_FLAG, pdTRUE, pdTRUE, portMAX_DELAY);

        getCoordinates(&lat, &lon);
        snprintf(forecast, sizeof(forecast), "http://api.openweathermap.org/data/2.5/onecall?lat=%.6f&lon=%.6f&exclude=current,minutely,hourly,alerts&appid=%s&units=metric", lat, lon, api_key);
        http.begin(forecast);
        http_code = http.GET();
        http.getString().toCharArray(payload, sizeof(payload));
        deserializeJson(doc, payload);
        File data = FFat.open("/forecast.json", "w", true);
        data.print(doc.as<String>());
        data.flush(); // guarantee that the data is written to the file
        data.close();
        http.end();

        Serial.println("Done getting forecast weather");
        xEventGroupSetBits(WiFi_EventGroup, DONE_USING_WIFI_FLAG);
        xEventGroupSetBits(GetWeather_EventGroup, DONE_GET_FORECAST_WEATHER_FLAG);
        taskYIELD();
    }
}
void ProcessingForecastWeather(void *pvParameters)
{
    for (;;)
    {
        xEventGroupWaitBits(GetWeather_EventGroup, DONE_GET_FORECAST_WEATHER_FLAG, pdTRUE, pdTRUE, portMAX_DELAY);
        File data = FFat.open("/forecast.json", "r");
        PSRAMJsonDocument doc(5120);
        deserializeJson(doc, data.readString());
        forecast **forecast_data = (forecast **)ps_malloc(sizeof(forecast *) * FORECAST_NUMS);
        for (int i = 0; i < 8; i++)
        {
            forecast_data[i] = (forecast *)ps_malloc(sizeof(forecast));
            forecast_data[i]->temp_min = doc["daily"][i]["temp"]["min"];
            forecast_data[i]->temp_max = doc["daily"][i]["temp"]["max"];
            doc["daily"][i]["weather"][0]["icon"].as<String>().toCharArray(forecast_data[i]->icon, 3);
            xQueueSend(forecast_queue, forecast_data[i], portMAX_DELAY);
        }
        free(forecast_data); // free the memory
        data.close();
        taskYIELD();
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
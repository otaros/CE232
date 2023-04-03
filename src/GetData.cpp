#include "GetData.h"

void GetCurrentWeather(void *pvParameters)
{
    char location_url[128];
    char weather_url[128];
    char payload[640];
    PSRAMJsonDocument doc(1024);
    File data;
    int http_code;
    for (;;)
    {
        // get location phase
        xEventGroupWaitBits(GetData_EventGroup, START_GET_CURRENT_WEATHER_FLAG, pdFALSE, pdFALSE, portMAX_DELAY);
        Serial.println("Start getting current weather");
        xEventGroupSetBits(WiFi_EventGroup, REQUEST_WIFI_FLAG);
        Serial.println("Requesting wifi connection (Current Weather)");
        xEventGroupWaitBits(WiFi_EventGroup, WIFI_IS_AVAILABLE_FOR_USE_FLAG, pdFALSE, pdFALSE, portMAX_DELAY);

        xSemaphoreTake(coordinate_mutex, portMAX_DELAY); // take the mutex
        getCoordinates(&lat, &lon);
        xSemaphoreGive(coordinate_mutex); // give the mutex

        snprintf(location_url, sizeof(location_url), "http://api.openweathermap.org/geo/1.0/reverse?lat=%.6f&lon=%.6f&limit=1&appid=%s", lat, lon, api_key);
        // Serial.println(location); // for checking purpose
        http1.begin(location_url);
        http_code = http1.GET();
        deserializeJson(doc, http1.getString());
        http1.end();
        doc[0]["name"].as<String>().toCharArray(city_name, sizeof(city_name));
        // Serial.println(city_name);
        // get data phase
        snprintf(weather_url, sizeof(weather_url), "http://api.openweathermap.org/data/2.5/weather?q=%s&appid=%s&units=metric", city_name, api_key);
        // Serial.println(weather); // for checking purpose
        http1.begin(weather_url);
        http_code = http1.GET();
        http1.getString().toCharArray(payload, sizeof(payload));
        http1.end();
        Serial.println(payload); // show the payload
        deserializeJson(doc, payload);
        data = FFat.open("/current.json", "w", true);
        data.print(doc.as<String>());
        data.flush();
        data.close();

        Serial.println("Done getting current weather");
        xEventGroupSetBits(WiFi_EventGroup, DONE_USING_WIFI_FLAG);
        xEventGroupClearBits(GetData_EventGroup, START_GET_CURRENT_WEATHER_FLAG);
        xEventGroupSetBits(GetData_EventGroup, DONE_GET_CURRENT_WEATHER_FLAG);
        taskYIELD();
    }
}
void ProcessingCurrentWeather(void *pvParameters)
{
    PSRAMJsonDocument doc(1024);
    BaseType_t ret;
    File data;
    for (;;)
    {
        // Processing current weather
        xEventGroupWaitBits(GetData_EventGroup, DONE_GET_CURRENT_WEATHER_FLAG, pdTRUE, pdFALSE, portMAX_DELAY);
        Serial.println("Start processing current weather");
        data = FFat.open("/current.json", "r");
        deserializeJson(doc, data.readString());
        // Serial.println(doc.as<String>());

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

        ret = xQueueSend(current_weather_queue, current, portMAX_DELAY);
        while (ret != pdTRUE)
        {
            ret = xQueueSend(current_weather_queue, current, portMAX_DELAY);
        }

        Serial.println("Done processing current weather");
        xEventGroupSetBits(GetData_EventGroup, DONE_PROCESSING_CURRENT_WEATHER_FLAG);
        taskYIELD(); // finish the task
    }
}
void GetForecastWeather(void *pvParameters) // get 8-day forecast
{
    char forecast_url[128];
    char payload[4096];
    PSRAMJsonDocument doc(5120);
    File data;
    int http_code;
    for (;;)
    {
        // get data phase
        xEventGroupWaitBits(GetData_EventGroup, START_GET_FORECAST_WEATHER_FLAG, pdFALSE, pdFALSE, portMAX_DELAY);
        Serial.println("Start getting forecast weather");
        xEventGroupSetBits(WiFi_EventGroup, REQUEST_WIFI_FLAG);
        Serial.println("Requesting wifi connection (Forecast Weather))");
        xEventGroupWaitBits(WiFi_EventGroup, WIFI_IS_AVAILABLE_FOR_USE_FLAG, pdFALSE, pdFALSE, portMAX_DELAY);

        xSemaphoreTake(coordinate_mutex, portMAX_DELAY); // take the mutex
        getCoordinates(&lat, &lon);
        xSemaphoreGive(coordinate_mutex); // give the mutex

        snprintf(forecast_url, sizeof(forecast_url), "http://api.openweathermap.org/data/2.5/onecall?lat=%.6f&lon=%.6f&exclude=current,minutely,hourly,alerts&appid=%s&units=metric", lat, lon, api_key);
        http2.begin(forecast_url);
        http_code = http2.GET();
        http2.getString().toCharArray(payload, sizeof(payload));
        http2.end();
        deserializeJson(doc, payload);
        data = FFat.open("/forecast.json", "w", true);
        data.print(doc.as<String>());
        data.flush(); // guarantee that the data is written to the file
        data.close();

        Serial.println("Done getting forecast weather");
        xEventGroupSetBits(WiFi_EventGroup, DONE_USING_WIFI_FLAG);
        xEventGroupClearBits(GetData_EventGroup, START_GET_FORECAST_WEATHER_FLAG);
        xEventGroupSetBits(GetData_EventGroup, DONE_GET_FORECAST_WEATHER_FLAG);
        taskYIELD();
    }
}
void ProcessingForecastWeather(void *pvParameters)
{
    PSRAMJsonDocument doc(5120);
    File data;
    for (;;)
    {
        xEventGroupWaitBits(GetData_EventGroup, DONE_GET_FORECAST_WEATHER_FLAG, pdTRUE, pdFALSE, portMAX_DELAY);
        data = FFat.open("/forecast.json", "r");
        deserializeJson(doc, data.readString());
        xQueueReset(forecast_queue);
        forecast **forecast_data = (forecast **)ps_malloc(sizeof(forecast *) * FORECAST_NUMS);
        for (int i = 0; i < 8; i++)
        {
            forecast_data[i] = (forecast *)ps_malloc(sizeof(forecast));
            forecast_data[i]->dt = doc["daily"][i]["dt"];
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
void GetAQI(void *pvParameters)
{
    char aqi_url[128];
    PSRAMJsonDocument doc(1024);
    int http_code;
    for (;;)
    {
        xEventGroupWaitBits(GetData_EventGroup, START_GET_AQI_FLAG, pdFALSE, pdFALSE, portMAX_DELAY);
        Serial.println("Start getting current weather");
        xEventGroupSetBits(WiFi_EventGroup, REQUEST_WIFI_FLAG);
        Serial.println("Requesting wifi connection (AQI)");
        xEventGroupWaitBits(WiFi_EventGroup, WIFI_IS_AVAILABLE_FOR_USE_FLAG, pdFALSE, pdFALSE, portMAX_DELAY);

        xSemaphoreTake(coordinate_mutex, portMAX_DELAY); // take the mutex
        getCoordinates(&lat, &lon);
        xSemaphoreGive(coordinate_mutex); // give the mutex

        snprintf(aqi_url, sizeof(aqi_url), "http://api.openweathermap.org/data/2.5/air_pollution?lat=%.6f&lon=%.6f&appid=%s", lat, lon, api_key);
        http3.begin(aqi_url);
        http_code = http3.GET();
        deserializeJson(doc, http3.getString());
        http3.end();
        aqi = doc["list"][0]["main"]["aqi"];
        Serial.printf("AQI: %d\n", aqi);

        Serial.println("Done getting AQI");
        xEventGroupClearBits(GetData_EventGroup, START_GET_AQI_FLAG);
        xEventGroupSetBits(WiFi_EventGroup, DONE_USING_WIFI_FLAG);
        xEventGroupSetBits(GetData_EventGroup, DONE_GET_AQI_FLAG);
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
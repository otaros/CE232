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
        if (xEventGroupGetBits(GetData_EventGroup) & START_GET_CURRENT_WEATHER_FLAG)
        {
        }
        else
        {
            continue;
        }

        xEventGroupSetBits(WiFi_EventGroup, REQUEST_WIFI_FLAG);
        Serial.println("Requesting wifi connection (Current Weather)");
        xEventGroupWaitBits(WiFi_EventGroup, WIFI_IS_AVAILABLE_FOR_USE_FLAG, pdFALSE, pdFALSE, portMAX_DELAY);

        snprintf(weather_url, sizeof(weather_url), "http://api.openweathermap.org/data/2.5/weather?lat=%.6f&lon=%.6f&appid=%s&units=metric", lat, lon, openweather_api_key);
        xSemaphoreTake(http_mutex, portMAX_DELAY); // take the mutex
        Serial.println("Start getting current weather");
        // Serial.println(weather_url); // for checking purpose
        http.begin(weather_url);
        while (http.GET() != HTTP_CODE_OK)
        {
            delay(100);
        }
        http.getString().toCharArray(payload, sizeof(payload));
        http.end();
        xSemaphoreGive(http_mutex); // give the mutex
        // Serial.println(payload);    // show the payload
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
        xEventGroupWaitBits(GetData_EventGroup, DONE_GET_CURRENT_WEATHER_FLAG, pdFALSE, pdFALSE, portMAX_DELAY);
        if (xEventGroupGetBits(GetData_EventGroup) & DONE_GET_CURRENT_WEATHER_FLAG)
        {
        }
        else
        {
            continue;
        }

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
        current->sunset = doc["sys"]["sunset"];
        doc["weather"][0]["icon"].as<String>().toCharArray(current->icon, 4);
        data.close();

        ret = xQueueSend(current_weather_queue, current, portMAX_DELAY);
        while (ret != pdTRUE)
        {
            ret = xQueueSend(current_weather_queue, current, portMAX_DELAY);
        }

        Serial.println("Done processing current weather");
        xEventGroupSetBits(GetData_EventGroup, DONE_PROCESSING_CURRENT_WEATHER_FLAG);
        xEventGroupClearBits(GetData_EventGroup, DONE_GET_CURRENT_WEATHER_FLAG);
        taskYIELD(); // finish the task
    }
}
void GetForecastWeather(void *pvParameters) // get 8-day forecast
{
    char url[128];
    char payload[4096];
    PSRAMJsonDocument doc(5120);
    File data;
    for (;;)
    {
        // get data phase
        xEventGroupWaitBits(GetData_EventGroup, START_GET_FORECAST_WEATHER_FLAG, pdFALSE, pdFALSE, portMAX_DELAY);
        if (xEventGroupGetBits(GetData_EventGroup) & START_GET_FORECAST_WEATHER_FLAG)
        {
        }
        else
        {
            continue;
        }

        xEventGroupSetBits(WiFi_EventGroup, REQUEST_WIFI_FLAG);
        Serial.println("Requesting wifi connection (Forecast Weather))");
        xEventGroupWaitBits(WiFi_EventGroup, WIFI_IS_AVAILABLE_FOR_USE_FLAG, pdFALSE, pdFALSE, portMAX_DELAY);

        snprintf(url, sizeof(url), "http://api.openweathermap.org/data/2.5/onecall?lat=%.6f&lon=%.6f&exclude=current,minutely,hourly,alerts&appid=%s&units=metric", lat, lon, openweather_api_key);
        xSemaphoreTake(http_mutex, portMAX_DELAY); // take the mutex
        Serial.println("Start getting forecast weather");
        http.begin(url);
        while (http.GET() != HTTP_CODE_OK)
        {
            delay(100);
        }
        http.getString().toCharArray(payload, sizeof(payload));
        http.end();
        xSemaphoreGive(http_mutex); // give the mutex
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
        xEventGroupWaitBits(GetData_EventGroup, DONE_GET_FORECAST_WEATHER_FLAG, pdFALSE, pdFALSE, portMAX_DELAY);
        if (xEventGroupGetBits(GetData_EventGroup) & DONE_GET_FORECAST_WEATHER_FLAG)
        {
        }
        else
        {
            continue;
        }
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
            doc["daily"][i]["weather"][0]["icon"].as<String>().toCharArray(forecast_data[i]->icon, 4);
            xQueueSend(forecast_queue, forecast_data[i], portMAX_DELAY);
        }
        free(forecast_data); // free the memory
        data.close();
        Serial.println("Done processing forecast weather");
        xEventGroupSetBits(GetData_EventGroup, DONE_PROCESSING_FORECAST_WEATHER_FLAG);
        xEventGroupClearBits(GetData_EventGroup, DONE_GET_FORECAST_WEATHER_FLAG);
        taskYIELD();
    }
}
void GetAQI(void *pvParameters)
{
    char url[128];
    PSRAMJsonDocument doc(1024);
    for (;;)
    {
        xEventGroupWaitBits(GetData_EventGroup, START_GET_AQI_FLAG, pdFALSE, pdFALSE, portMAX_DELAY);
        if (xEventGroupGetBits(GetData_EventGroup) & START_GET_AQI_FLAG)
        {
        }
        else
        {
            continue;
        }

        Serial.println("Start getting current weather");
        xEventGroupSetBits(WiFi_EventGroup, REQUEST_WIFI_FLAG);
        Serial.println("Requesting wifi connection (AQI)");
        xEventGroupWaitBits(WiFi_EventGroup, WIFI_IS_AVAILABLE_FOR_USE_FLAG, pdFALSE, pdFALSE, portMAX_DELAY);

        snprintf(url, sizeof(url), "http://api.openweathermap.org/data/2.5/air_pollution?lat=%.6f&lon=%.6f&appid=%s", lat, lon, openweather_api_key);
        xSemaphoreTake(http_mutex, portMAX_DELAY); // take the mutex
        http.begin(url);
        while (http.GET() != HTTP_CODE_OK)
        {
            delay(100);
        }
        deserializeJson(doc, http.getString());
        http.end();
        xSemaphoreGive(http_mutex); // give the mutex
        aqi = doc["list"][0]["main"]["aqi"];
        Serial.printf("AQI: %d\n", aqi);

        Serial.println("Done getting AQI");
        xEventGroupClearBits(GetData_EventGroup, START_GET_AQI_FLAG);
        xEventGroupSetBits(WiFi_EventGroup, DONE_USING_WIFI_FLAG);
        xEventGroupSetBits(GetData_EventGroup, DONE_GET_AQI_FLAG);
        taskYIELD();
    }
}

void GetUV(void *pvParameters)
{
    char url[128];
    PSRAMJsonDocument doc(2048);
    for (;;)
    {
        xEventGroupWaitBits(GetData_EventGroup, START_GET_UV_FLAG, pdFALSE, pdFALSE, portMAX_DELAY);
        if (xEventGroupGetBits(GetData_EventGroup) & START_GET_UV_FLAG)
        {
        }
        else
        {
            continue;
        }

        xEventGroupSetBits(WiFi_EventGroup, REQUEST_WIFI_FLAG);
        Serial.println("Requesting wifi connection (UV index)");
        xEventGroupWaitBits(WiFi_EventGroup, WIFI_IS_AVAILABLE_FOR_USE_FLAG, pdFALSE, pdFALSE, portMAX_DELAY);

        snprintf(url, sizeof(url), "http://api.openuv.io/api/v1/uv?lat=%.6f&lng=%.6f&", lat, lon);
        xSemaphoreTake(http_mutex, portMAX_DELAY); // take the mutex
        Serial.println("Start getting UV index");
        http.begin(url);
        http.addHeader("x-access-token", uv_api_key);
        http.addHeader("Content-Type", "application/json");
        while (http.GET() != HTTP_CODE_OK)
        {
            delay(100);
        }
        deserializeJson(doc, http.getString());
        http.end();
        xSemaphoreGive(http_mutex); // give the mutex
        uv = doc["result"]["uv"];
        Serial.printf("UV index: %.2f\n", uv);

        Serial.println("Done getting UV index");
        xEventGroupClearBits(GetData_EventGroup, START_GET_UV_FLAG);
        xEventGroupSetBits(WiFi_EventGroup, DONE_USING_WIFI_FLAG);
        xEventGroupSetBits(GetData_EventGroup, DONE_GET_UV_FLAG);
        taskYIELD();
    }
}

void Get3_HoursForecastWeather(void *pvParameters)
{
    char url[128];
    char payload[8192];
    PSRAMJsonDocument doc(4096);
    for (;;)
    {
        xEventGroupWaitBits(GetData_EventGroup, START_GET_3HOURS_FORECAST_FLAG, pdFALSE, pdFALSE, portMAX_DELAY);
        if (xEventGroupGetBits(GetData_EventGroup) & START_GET_3HOURS_FORECAST_FLAG)
        {
        }
        else
        {
            continue;
        }
        Serial.println("Start getting 3-Hours Forecast");
        xEventGroupSetBits(WiFi_EventGroup, REQUEST_WIFI_FLAG);
        Serial.println("Requesting wifi connection (3-Hours Forecast)");
        xEventGroupWaitBits(WiFi_EventGroup, WIFI_IS_AVAILABLE_FOR_USE_FLAG, pdFALSE, pdFALSE, portMAX_DELAY);

        snprintf(url, sizeof(url), "http://api.openweathermap.org/data/2.5/forecast?lat=%.6f&lon=%.6f&appid=%s&units=metric&cnt=6", lat, lon, openweather_api_key);
        xSemaphoreTake(http_mutex, portMAX_DELAY); // take the mutex
        http.begin(url);
        while (http.GET() != HTTP_CODE_OK)
        {
            // add some error handling here
            delay(100);
        }
        http.getString().toCharArray(payload, sizeof(payload));
        http.end();
        xSemaphoreGive(http_mutex); // give the mutex
        deserializeJson(doc, payload);
        File data = FFat.open("/3hours_forecast.json", "w", true);
        data.print(doc.as<String>());
        data.flush();
        data.close();

        Serial.println("Done get 3-Hours Forecast");
        xEventGroupClearBits(GetData_EventGroup, START_GET_3HOURS_FORECAST_FLAG);
        xEventGroupSetBits(WiFi_EventGroup, DONE_USING_WIFI_FLAG);
        xEventGroupSetBits(GetData_EventGroup, DONE_GET_3HOURS_FORECAST_FLAG);
        taskYIELD();
    }
}

void Processing3_HoursForecastWeather(void *pvParameters)
{
    PSRAMJsonDocument doc(4096);
    File data;
    for (;;)
    {
        xEventGroupWaitBits(GetData_EventGroup, DONE_GET_3HOURS_FORECAST_FLAG, pdFALSE, pdFALSE, portMAX_DELAY);

        data = FFat.open("/3hours_forecast.json", "r");
        Serial.println("Start processing 3-Hours Forecast");
        deserializeJson(doc, data.readString());
        xQueueReset(three_hours_forecast_queue);
        forecast **forecast_data = (forecast **)ps_malloc(sizeof(forecast *) * 6);
        for (int i = 0; i < 6; i++)
        {
            forecast_data[i] = (forecast *)ps_malloc(sizeof(forecast));
            forecast_data[i]->dt = doc["list"][i]["dt"];
            forecast_data[i]->temp_min = doc["list"][i]["main"]["temp_min"];
            forecast_data[i]->temp_max = doc["list"][i]["main"]["temp_max"];
            doc["list"][i]["weather"][0]["icon"].as<String>().toCharArray(forecast_data[i]->icon, 4);
            xQueueSend(three_hours_forecast_queue, &forecast_data[i], portMAX_DELAY);
        }
        free(forecast_data);
        data.close();
        Serial.println("Done processing 3-Hours Forecast");
        xEventGroupSetBits(GetData_EventGroup, DONE_PROCESSING_3HOURS_FORECAST_FLAG);
        xEventGroupClearBits(GetData_EventGroup, DONE_GET_3HOURS_FORECAST_FLAG);
        taskYIELD();
    }
}

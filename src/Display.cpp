#include "Display.h"

void DisplayTitle(void *pvParameters)
{
    xEventGroupWaitBits(GetData_EventGroup, DONE_GET_CURRENT_WEATHER_FLAG, pdFALSE, pdFALSE, portMAX_DELAY);
    for (;;)
    {
        title_Sprite.fillScreen(TFT_BLACK);
        getLocalTime(&structTime);
        strftime(date, sizeof(date), "%a, %d/%m/%Y, %H:%M:%S", &structTime);
        sprintf(title_text, "%s, %s", display_name, date);
        // Serial.println(title_text);
        title_Sprite.setCursor(0, 0);
        title_Sprite.println(title_text);
        title_Sprite.pushSprite(0, 0);
        delay(1000);
    }
}

void DisplayCurrentWeather(void *pvParameters)
{
    char filename[10];
    char temperture[10];
    BaseType_t ret;
    weather_data *data = (weather_data *)ps_malloc(sizeof(weather_data));
    current_weather_Sprite.setTextDatum(TC_DATUM);
    for (;;)
    {
        // xEventGroupWaitBits(GetData_EventGroup, DONE_PROCESSING_CURRENT_WEATHER_FLAG | DONE_GET_AQI_FLAG | DONE_GET_UV_FLAG, pdTRUE, pdTRUE, portMAX_DELAY);
        xEventGroupWaitBits(GetData_EventGroup, DONE_PROCESSING_CURRENT_WEATHER_FLAG | DONE_GET_AQI_FLAG, pdTRUE, pdTRUE, portMAX_DELAY);
        current_weather_Sprite.fillScreen(TFT_BLACK);
        Serial.println("Start displaying current weather");

        if (newScreen)
        {
            newScreen = false;
            continue;
        }
        if (uxQueueMessagesWaiting(current_weather_queue) != 0)
        {
            do
            {
                ret = xQueueReceive(current_weather_queue, data, pdTICKS_TO_MS(10));
                if (ret == pdTRUE)
                    xQueueReset(current_weather_queue);
            } while (ret != pdTRUE);
        }
        // 1.15 spacing for each line
        sprintf(filename, "/%s.bmp", data->icon);
        Serial.println(filename);

        drawBmpToSprite(filename, 48, 0, &current_weather_Sprite);
        current_weather_Sprite.setCursor(60, 55);
        current_weather_Sprite.printf("%.2f°C", data->temp);

        drawBmpToSprite("/thermometer.bmp", 27, 65, &current_weather_Sprite);
        current_weather_Sprite.setCursor(40, 67);
        current_weather_Sprite.printf("%.2f°C/%.2f°C", data->temp_min, data->temp_max);

        drawBmpToSprite("/water_drop.bmp", 27, 77, &current_weather_Sprite);
        current_weather_Sprite.setCursor(40, 79);
        current_weather_Sprite.printf("%d%%", data->humidity);

        drawBmpToSprite("/pressure.bmp", 26, 89, &current_weather_Sprite);
        current_weather_Sprite.setCursor(40, 91);
        current_weather_Sprite.printf("%d hPa", data->pressure);

        drawBmpToSprite("/wind.bmp", 25, 101, &current_weather_Sprite);
        current_weather_Sprite.setCursor(40, 103);
        current_weather_Sprite.printf("%.2f km/h %s", data->wind_speed * 3600 / 1000, degToCompass(data->wind_deg));

        drawBmpToSprite("/uv.bmp", 26, 113, &current_weather_Sprite);
        if (uv < 10)
        {

            current_weather_Sprite.fillRoundRect(40, 113, 21, 11, 1, getColorUV(uv));
        }
        else
        {
            current_weather_Sprite.fillRoundRect(40, 113, 27, 11, 1, getColorUV(uv));
        }
        current_weather_Sprite.setTextColor(TFT_BLACK);
        current_weather_Sprite.setCursor(40, 115);
        current_weather_Sprite.printf("%.2f", uv);
        current_weather_Sprite.setTextColor(TFT_WHITE);

        drawBmpToSprite("/sun.bmp", 26, 125, &current_weather_Sprite);
        current_weather_Sprite.setCursor(40, 127);
        current_weather_Sprite.printf("%02d:%02d/%02d:%02d", hour(data->sunrise + gmtOffset_sec), minute(data->sunrise), hour(data->sunset + gmtOffset_sec), minute(data->sunset));

        current_weather_Sprite.setCursor(40, 139);
        current_weather_Sprite.print("AQI: ");
        current_weather_Sprite.setTextColor(TFT_BLACK);
        current_weather_Sprite.fillRoundRect(63, 137, 6, 11, 1, getAQIColor(aqi));
        current_weather_Sprite.print(aqi);
        current_weather_Sprite.setTextColor(TFT_WHITE);

        // Serial.printf("%d, %d\n", current_weather_Sprite.getCursorX(), current_weather_Sprite.getCursorY());
        current_weather_Sprite.pushSprite(40, 5, TFT_BLACK);
        // taskYIELD();
    }
}

// void DisplayForecastWeather(void *pvParameters)
// {
//     for (;;)
//     {
//         xEventGroupWaitBits(GetData_EventGroup, DONE_PROCESSING_FORECAST_WEATHER_FLAG, pdTRUE, pdFALSE, portMAX_DELAY);
//         delay(500);
//     }
// }

void DisplayThreeHoursForecast(void *pvParameters)
{
    BaseType_t ret;
    forecast *data = (forecast *)ps_malloc(sizeof(forecast));
    char filename[10];
    char time[10];
    for (;;)
    {
        xEventGroupWaitBits(GetData_EventGroup, DONE_PROCESSING_3HOURS_FORECAST_FLAG, pdTRUE, pdFALSE, portMAX_DELAY);
        forecast3hours_weather_Sprite.fillSprite(TFT_BLACK);
        Serial.println("Start displaying 3 hours forecast");
        // forecast3hours_weather_Sprite.setTextDatum(MC_DATUM);
        // forecast3hours_weather_Sprite.drawString("3 hours forecast", tft.width() / 2, 13, 0);
        tft.setTextDatum(MC_DATUM);
        tft.drawString(display_name, tft.width() / 2, 11, 0);
        tft.setTextDatum(TL_DATUM);

        if (uxQueueMessagesWaiting(three_hours_forecast_queue) != 0)
        {
            for (uint8_t i = 0; i < 6; i++)
            {
                do
                {
                    ret = xQueueReceive(three_hours_forecast_queue, data, pdTICKS_TO_MS(10));
                } while (ret != pdTRUE);

                forecast3hours_weather_Sprite.fillScreen(TFT_BLACK);
                sprintf(filename, "/%s.bmp", data->icon);
                Serial.println(filename);
                drawBmpToSprite(filename, 28, 0, &forecast3hours_weather_Sprite);
                forecast3hours_weather_Sprite.setTextDatum(MC_DATUM);
                sprintf(time, "%02d:%02d", hour(data->dt + gmtOffset_sec), minute(data->dt));
                forecast3hours_weather_Sprite.drawString(time, forecast3hours_weather_Sprite.width() / 2, 11, 0);

                drawBmpToSprite("/thermometer.bmp", 14, 49, &forecast3hours_weather_Sprite);
                forecast3hours_weather_Sprite.setCursor(28, 50);
                forecast3hours_weather_Sprite.printf("%.2f°C/%.2f°C", data->temp_min, data->temp_max);

                drawBmpToSprite("/water_drop.bmp", 14, 63, &forecast3hours_weather_Sprite);
                forecast3hours_weather_Sprite.setCursor(28, 64);
                forecast3hours_weather_Sprite.printf("%d%%", data->humidity);

                forecast3hours_weather_Sprite.pushSprite((i % 2 == 0 ? 0 : 120), (i == 0 || i == 1 ? 20 : 100 * (i - 1) + 10) - (i > 2 & i % 2 == 1 ? 100 : 0) - (i > 3 ? 100 : 0), TFT_BLACK);
            }
        }

        xEventGroupClearBits(GetData_EventGroup, DONE_PROCESSING_3HOURS_FORECAST_FLAG);

        delay(500);
    }
}

void displayMenu()
{
    Menu_Sprite.setCursor(0, 0);
    Menu_Sprite.println("Return to main");
    Menu_Sprite.println("Get 3-hours Forecast");
    Menu_Sprite.println("Input API key");
    Menu_Sprite.println("Change location");
    Menu_Sprite.println("Change WiFi connection");
    Menu_Sprite.pushSprite(15, 0);
}

uint16_t getColorUV(double uv)
{
    if (uv < 3)
    {
        return TFT_GREEN;
    }
    else if (uv < 6)
    {
        return TFT_YELLOW;
    }
    else if (uv < 8)
    {
        return TFT_ORANGE;
    }
    else if (uv < 11)
    {
        return TFT_RED;
    }
    else
    {
        return TFT_PURPLE;
    }
}

uint16_t getAQIColor(uint8_t aqi)
{
    switch (aqi)
    {
    case 1:
        return TFT_GREEN;
    case 2:
        return TFT_YELLOW;
    case 3:
        return TFT_ORANGE;
    case 4:
        return TFT_RED;
    case 5:
        return TFT_PURPLE;
    default:
        return TFT_BLACK;
    }
}
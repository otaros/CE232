#include "Display.h"

void DisplayTitle(void *pvParameters)
{
    xEventGroupWaitBits(GetData_EventGroup, DONE_GET_CURRENT_WEATHER_FLAG, pdFALSE, pdFALSE, portMAX_DELAY);
    for (;;)
    {
        title_Sprite.fillScreen(TFT_BLACK);
        getLocalTime(&structTime);
        strftime(date, sizeof(date), "%a, %d/%m/%Y, %H:%M:%S", &structTime);
        sprintf(title_text, "%s, %s", city_name, date);
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
    current_weather_Sprite.setTextDatum(TC_DATUM);
    for (;;)
    {
        xEventGroupWaitBits(GetData_EventGroup, DONE_PROCESSING_CURRENT_WEATHER_FLAG, pdTRUE, pdFALSE, portMAX_DELAY);
        current_weather_Sprite.fillSprite(TFT_BLACK);
        Serial.println("Start displaying current weather");
        weather_data *data = (weather_data *)ps_malloc(sizeof(weather_data));
        ret = xQueueReceive(current_weather_queue, data, portMAX_DELAY);
        while (ret != pdTRUE)
        {
            ret = xQueueReceive(current_weather_queue, data, portMAX_DELAY);
        }
        // 1.15 spacing for each line
        sprintf(filename, "/%s.bmp", data->icon);
        Serial.println(filename);
        current_weather_Sprite.fillSprite(TFT_BLACK);

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
        current_weather_Sprite.printf("%.2f km/h", data->wind_speed * 3600 / 1000);

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

        // Serial.printf("%d, %d\n", current_weather_Sprite.getCursorX(), current_weather_Sprite.getCursorY());
        current_weather_Sprite.pushSprite(40, 5, TFT_BLACK);
    }
}

void drawBmp(const char *filename, int16_t x, int16_t y)
{
    if ((x >= tft.width()) || (y >= tft.height()))
        return;

    fs::File bmpFS;

    // Open requested file on SD card
    bmpFS = FFat.open(filename, "r");

    if (!bmpFS)
    {
        Serial.print("File not found");
        return;
    }

    uint32_t seekOffset;
    uint16_t w, h, row, col;
    uint8_t r, g, b;

    uint32_t startTime = millis();

    if (read16(bmpFS) == 0x4D42)
    {
        read32(bmpFS);
        read32(bmpFS);
        seekOffset = read32(bmpFS);
        read32(bmpFS);
        w = read32(bmpFS);
        h = read32(bmpFS);

        if ((read16(bmpFS) == 1) && (read16(bmpFS) == 24) && (read32(bmpFS) == 0))
        {
            y += h - 1;

            bool oldSwapBytes = tft.getSwapBytes();
            tft.setSwapBytes(true);
            bmpFS.seek(seekOffset);

            uint16_t padding = (4 - ((w * 3) & 3)) & 3;
            uint8_t lineBuffer[w * 3 + padding];

            for (row = 0; row < h; row++)
            {

                bmpFS.read(lineBuffer, sizeof(lineBuffer));
                uint8_t *bptr = lineBuffer;
                uint16_t *tptr = (uint16_t *)lineBuffer;
                // Convert 24 to 16 bit colours
                for (uint16_t col = 0; col < w; col++)
                {
                    // bitmap file is stored in BGR format
                    b = *bptr++;
                    g = *bptr++;
                    r = *bptr++;
                    *tptr++ = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
                }

                // Push the pixel row to screen, pushImage will crop the line if needed
                // y is decremented as the BMP image is drawn bottom up
                tft.pushImage(x, y--, w, 1, (uint16_t *)lineBuffer);
            }
            tft.setSwapBytes(oldSwapBytes);
            Serial.print("Loaded in ");
            Serial.print(millis() - startTime);
            Serial.println(" ms");
        }
        else
            Serial.println("BMP format not recognized.");
    }
    bmpFS.close();
}

void drawBmpToSprite(const char *filename, int16_t x, int16_t y, TFT_eSprite *spr)
{

    if ((x >= spr->width()) || (y >= spr->height()))
        return;

    fs::File bmpFS;

    // Open requested file on SD card
    bmpFS = FFat.open(filename, "r");

    if (!bmpFS)
    {
        Serial.print("File not found");
        return;
    }

    uint32_t seekOffset;
    uint16_t w, h, row, col;
    uint8_t r, g, b;

    uint32_t startTime = millis();

    if (read16(bmpFS) == 0x4D42)
    {
        read32(bmpFS);
        read32(bmpFS);
        seekOffset = read32(bmpFS);
        read32(bmpFS);
        w = read32(bmpFS);
        h = read32(bmpFS);

        if ((read16(bmpFS) == 1) && (read16(bmpFS) == 24) && (read32(bmpFS) == 0))
        {
            y += h - 1;

            bool oldSwapBytes = spr->getSwapBytes();
            spr->setSwapBytes(true);
            bmpFS.seek(seekOffset);

            uint16_t padding = (4 - ((w * 3) & 3)) & 3;
            uint8_t lineBuffer[w * 3 + padding];

            for (row = 0; row < h; row++)
            {

                bmpFS.read(lineBuffer, sizeof(lineBuffer));
                uint8_t *bptr = lineBuffer;
                uint16_t *tptr = (uint16_t *)lineBuffer;
                // Convert 24 to 16 bit colours
                for (uint16_t col = 0; col < w; col++)
                {
                    // bitmap file is stored in BGR format
                    b = *bptr++;
                    g = *bptr++;
                    r = *bptr++;
                    *tptr++ = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
                }

                // Push the pixel row to screen, pushImage will crop the line if needed
                // y is decremented as the BMP image is drawn bottom up
                spr->pushImage(x, y--, w, 1, (uint16_t *)lineBuffer);
            }
            spr->setSwapBytes(oldSwapBytes);
            Serial.print("Loaded in ");
            Serial.print(millis() - startTime);
            Serial.println(" ms");
        }
        else
            Serial.println("BMP format not recognized.");
    }
    bmpFS.close();
}

uint16_t read16(fs::File &f)
{
    uint16_t result;
    ((uint8_t *)&result)[0] = f.read(); // LSB
    ((uint8_t *)&result)[1] = f.read(); // MSB
    return result;
}

uint32_t read32(fs::File &f)
{
    uint32_t result;
    ((uint8_t *)&result)[0] = f.read(); // LSB
    ((uint8_t *)&result)[1] = f.read();
    ((uint8_t *)&result)[2] = f.read();
    ((uint8_t *)&result)[3] = f.read(); // MSB
    return result;
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
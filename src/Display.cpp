#include "Display.h"

void DisplayTitle(void *pvParameters)
{
    xEventGroupWaitBits(GetWeather_EventGroup, DONE_GET_CURRENT_WEATHER_FLAG, pdTRUE, pdTRUE, portMAX_DELAY);
    for (;;)
    {
        title.fillScreen(TFT_BLACK);
        getLocalTime(&structTime);
        strftime(date, sizeof(date), "%A, %d/%m/%Y, %H:%M:%S", &structTime);
        sprintf(title_text, "%s, %s", city_name, date);
        Serial.println(title_text);
        title.setCursor(0, 0);
        title.println(title_text);
        title.pushSprite(0, 0);
        delay(1000);
    }
}

void DisplayCurrentWeather(void *pvParameters)
{
    char filename[10];
    for (;;)
    {
        xEventGroupWaitBits(GetWeather_EventGroup, DONE_PROCESSING_CURRENT_WEATHER_FLAG, pdTRUE, pdTRUE, portMAX_DELAY);
        Serial.println("Start displaying current weather");
        weather_data *data = (weather_data *)ps_malloc(sizeof(weather_data));
        BaseType_t ret = xQueueReceive(current_weather_queue, data, portMAX_DELAY);
        while (ret != pdTRUE)
        {
            ret = xQueueReceive(current_weather_queue, data, portMAX_DELAY);
        }
        sprintf(filename, "/%s.bmp", data->icon);
        Serial.println(filename);
        drawBmp(filename, 128, 50);
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
            // uint8_t *lineBuffer = (uint8_t *)ps_malloc(sizeof(uint8_t) * (w * 3 + padding));

            for (row = 0; row < h; row++)
            {

                bmpFS.read(lineBuffer, sizeof(lineBuffer));
                uint8_t *bptr = lineBuffer;
                uint16_t *tptr = (uint16_t *)lineBuffer;
                // Convert 24 to 16 bit colours
                for (uint16_t col = 0; col < w; col++)
                {
                    r = *bptr++;
                    g = *bptr++;
                    b = *bptr++;
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
#include "Utilities.h"

BUTTON_STATE getButtonState()
{
    if (digitalRead(KEY) == LOW)
    {
        ulong start = millis();
        while (digitalRead(KEY) == LOW)
        {
            if (millis() - start > 400 && digitalRead(KEY) == HIGH)
            {
                return KEY_HOLD;
            }
        }
        return KEY_PRESSED;
    }
    else if (digitalRead(UP_KEY) == LOW)
    {
        while (digitalRead(UP_KEY) == LOW)
            ;
        return UP;
    }
    else if (digitalRead(DOWN_KEY) == LOW)
    {
        while (digitalRead(DOWN_KEY) == LOW)
            ;
        return DOWN;
    }
    else if (digitalRead(LEFT_KEY) == LOW)
    {
        while (digitalRead(LEFT_KEY) == LOW)
            ;
        return LEFT;
    }
    else if (digitalRead(RIGHT_KEY) == LOW)
    {
        while (digitalRead(RIGHT_KEY) == LOW)
            ;
        return RIGHT;
    }
    return NONE;
}

String degToCompass(uint16_t num)
{
    String compassPoints[] = {"N", "NNE", "NE", "ENE", "E", "ESE", "SE",
                              "SSE", "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW"};
    float_t var = (num / 22.5) + 0.5;
    return compassPoints[(uint16_t)var % 16];
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

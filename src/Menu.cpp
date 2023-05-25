#include "Menu.h"

int8_t menu_index = 0;

void MenuControl(void *pvParameters)
{
    xEventGroupWaitBits(CurrentFlow_EventGroup, MENU, pdFALSE, pdFALSE, portMAX_DELAY);
    drawBmpToSprite("/cursor.bmp", 0, 0, &menu_cursor_Sprite);
    menu_cursor_Sprite.pushSprite(0, 0);
    uint8_t menu_index_old = 0;
    boolean re_render = false;
    for (;;)
    {
        if (xEventGroupGetBits(CurrentFlow_EventGroup) == MENU)
        {
            if (re_render)
            {
                menu_cursor_Sprite.fillSprite(TFT_BLACK);
                drawBmpToSprite("/cursor.bmp", 0, menu_index * 11, &menu_cursor_Sprite);
                menu_cursor_Sprite.pushSprite(0, 0);
                re_render = false;
            }
            else if (menu_index != menu_index_old)
            {
                menu_cursor_Sprite.fillSprite(TFT_BLACK);
                drawBmpToSprite("/cursor.bmp", 0, menu_index * 11, &menu_cursor_Sprite);
                menu_cursor_Sprite.pushSprite(0, 0);
                menu_index_old = menu_index;
            }
            else if (getButtonState() == KEY_PRESSED)
            {
                switch (menu_index)
                {
                case 0:
                    Serial.println("MAIN HOME");
                    xEventGroupSetBits(WorkingFlow_EventGroup, MAIN);
                    xEventGroupClearBits(CurrentFlow_EventGroup, 0xFFFF);
                    re_render = true;
                    break;
                case 1:
                    Serial.println("THREE_HOURS_FORECAST");
                    // re_render = true;
                    break;
                case 2:
                    Serial.println("INPUT_API_KEY");
                    // xEventGroupSetBits(WorkingFlow_EventGroup, INPUT_API);
                    // xEventGroupClearBits(CurrentFlow_EventGroup, 0xFFFF);
                    break;
                case 3:
                    Serial.println("CHANGE_LOCATION");
                    xEventGroupSetBits(WorkingFlow_EventGroup, CHANGE_LOCATION);
                    xEventGroupClearBits(CurrentFlow_EventGroup, 0xFFFF);
                    re_render = true;
                    break;
                case 4:
                    Serial.println("INPUT_WIFI_CREDENTIALS");
                    // re_render = true;
                    break;
                default:
                    break;
                }
            }
        }
    }
}

void ButtonMonitoring(void *pvParameters)
{
    BUTTON_STATE button;
    for (;;)
    {
        button = getButtonState();
        switch (button)
        {
        case KEY_HOLD:
            Serial.println("KEY_HOLD");
            xEventGroupSetBits(WorkingFlow_EventGroup, MENU);
            if (xEventGroupGetBits(CurrentFlow_EventGroup) != MENU)
            {
                xEventGroupClearBits(CurrentFlow_EventGroup, 0xFFFF);
            }
            break;
        case DOWN:
            if (xEventGroupGetBits(CurrentFlow_EventGroup) == MENU)
            {
                menu_index++;
                if (menu_index == NUMS_OPTION)
                {
                    menu_index = 0;
                }
            }
            Serial.printf("DOWN %d\n", menu_index);
            break;
        case UP:
            if (xEventGroupGetBits(CurrentFlow_EventGroup) == MENU)
            {
                menu_index--;
                if (menu_index < 0)
                {
                    menu_index = NUMS_OPTION - 1;
                }
            }
            Serial.printf("UP %d\n", menu_index);
            break;
        default:
            break;
        }
        delay(10);
    }
}
#include "Menu.h"

int8_t menu_index = 0;

void MenuControl(void *pvParameters)
{
    uint8_t menu_index_old = 0;
    for (;;)
    {
        if (xEventGroupGetBits(CurrentFlow_EventGroup) == MENU)
        {
            if (getButtonState() == KEY_PRESSED)
            {
                switch (menu_index)
                {
                case 0:
                    Serial.println("MAIN HOME");
                    xEventGroupSetBits(WorkingFlow_EventGroup, MAIN);
                    xEventGroupClearBits(CurrentFlow_EventGroup, 0xFFFF);
                    break;
                case 1:
                    Serial.println("THREE_HOURS_FORECAST");
                    break;
                case 2:
                    Serial.println("INPUT_API");
                    break;
                case 3:
                    Serial.println("INPUT_WIFI_CREDENTIALS");
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
                if (menu_index >= NUMS_OPTION)
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
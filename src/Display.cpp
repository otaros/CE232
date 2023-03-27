#include "Display.h"

void DisplayTitle()
{
    getLocalTime(&structTime);
    strftime(date, sizeof(date), "%A, %d/%m/%Y, %H:%M:%S", &structTime);
    sprintf(title_text, "%s, %s", city_name, date);
    title.printToSprite(title_text, sizeof(title_text));
    title.pushSprite(0, 0);
}
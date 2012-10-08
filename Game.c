#include "MeggyJrLib.h"

void
setAll(byte color)
{
    int             i,
                    j;
    for (i = 0; i < 8; ++i) {
        for (j = 0; j < 8; ++j) {
            DrawPx(i, j, color);
        }
    }
    DisplaySlate();
}

void
main(void)
{
    int             i = 0;
    SimpleInit();
//    SetAuxLEDsBinary(0 b01010101);
    ClearSlate();
    // DisplaySlate;
    while (1) {
        // setAll(Orange);
        // i = (i + 1) % 16;
        // delay(500);
        // setAll(Red);
        // delay(500);
        // setAll(Green);
        delay(500);
        /*
         * CheckButtonsPress(); if (Button_Up) { if (i > 0) --i;
         * setAll(i); } else if (Button_Down) { if (i < 10) ++i;
         * setAll(i); } delay(50); 
         */

    }
}

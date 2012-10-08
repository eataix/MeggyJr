#include "MeggyJrLib.h"

void
SetAll(byte colour)
{
    int             i,
                    j;
    for (i = 0; i < 8; ++i) {
        for (j = 0; j < 8; ++j) {
            Draw(i, j, colour);
        }
    }
    DisplaySlate();
}

void
main(void)
{
    int             i = 0;
    SimpleInit();
    // SetLedBinary(0b01010101);
    ClearSlate();
    DisplaySlate;
    ToneStart(ToneA6, 200);
    while (1) {
        SetLedBinary(0 b01010101);

        SetAll(Orange);
        Delay(1000);
        SetLedBinary(0 b10101010);
        SetAll(Red);
        Delay(1000);
        // SetAll(Green);
        // Delay(500);
    }
}

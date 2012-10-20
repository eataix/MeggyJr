#include "MeggyJrLib.h"
#include "MeggyJr.h"

byte            button_a;
byte            button_b;
byte            button_up;
byte            button_down;
byte            button_left;
byte            button_right;

static byte     game_slate[DIMENSION][DIMENSION];
static byte     last_button_state;


// Color lookup Table
byte            colour_table[26][3] = {
    {MeggyDark}
    ,
    {MeggyRed}
    ,
    {MeggyOrange}
    ,
    {MeggyYellow}
    ,
    {MeggyGreen}
    ,
    {MeggyBlue}
    ,
    {MeggyViolet}
    ,
    {MeggyWhite}
    ,
    {MeggyDimRed}
    ,
    {MeggyDimOrange}
    ,
    {MeggydimYellow}
    ,
    {MeggyDimGreen}
    ,
    {MeggydimAqua}
    ,
    {MeggyDimBlue}
    ,
    {MeggydimViolet}
    ,
    {MeggyCursorColor}
    ,
    {0, 0, 0}
    ,                           // CustomColor0 (dark, by default)
    {0, 0, 0}
    ,                           // CustomColor1 (dark, by default)
    {0, 0, 0}
    ,                           // CustomColor2 (dark, by default)
    {0, 0, 0}
    ,                           // CustomColor3 (dark, by default) 
    {0, 0, 0}
    ,                           // CustomColor4 (dark, by default)
    {0, 0, 0}
    ,                           // CustomColor5 (dark, by default)
    {0, 0, 0}
    ,                           // CustomColor6 (dark, by default)
    {0, 0, 0}
    ,                           // CustomColor7 (dark, by default)
    {0, 0, 0}
    ,                           // CustomColor8 (dark, by default)
    {0, 0, 0}                   // CustomColor9 (dark, by default)
};

void
CheckButtonDown(void)
{
    byte            i;
    i = GetButtons();

    button_b = (i & 1);
    button_a = (i & 2);
    button_up = (i & 4);
    button_down = (i & 8);
    button_left = (i & 16);
    button_right = (i & 32);

    last_button_state = i;
}

void
CheckButtonPressed(void)
{
    byte            j;
    byte            i = GetButtons();

    button_b = (i & 1);
    button_a = (i & 2);
    button_up = (i & 4);
    button_down = (i & 8);
    button_left = (i & 16);
    button_right = (i & 32);

    last_button_state = i;
}

void
SetLed(byte n)
{
    leds = n;
}

void
SetLedBinary(byte n)
{
    n = (n & 240) >> 4 | (n & 15) << 4;
    n = (n & 204) >> 2 | (n & 51) << 2;
    leds = (n & 170) >> 1 | (n & 85) << 1;
}

void
Draw(byte x, byte y, byte colour)
{
    game_slate[x][y] = colour;
}

void
SafeDraw(byte x, byte y, byte color)
{
    if ((x >= 0) && (x <= 7) && (y >= 0) && (y <= 7))
        game_slate[x][y] = color;
}

byte
ReadPixel(byte x, byte y)
{
    return game_slate[x][y];
}

void
ClearSlate(void)
{
    byte            i;
    byte            j;

    for (i = 0; i < 8; ++i) {
        for (j = 0; j < 8; ++j) {
            game_slate[i][j] = 0;
        }
    }
}

void
DisplaySlate(void)
{
    byte            i,
                    j;

    for (i = 0; i < 8; ++i) {
        for (j = 0; j < 8; ++j) {
            SetPixelColour(i, j, colour_table[game_slate[i][j]]);
        }
    }
}

void
ToneStart(unsigned int divisor, unsigned int duration_ms)
{
    StartTone(divisor, duration_ms);
}

void
SimpleInit(void)
{
    Init();
    ClearFrame();
    last_button_state = GetButtons();
    StartTone(0, 0);
    SoundOff();
}

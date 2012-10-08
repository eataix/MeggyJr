#include "MeggyJrLib.h"
#include "MeggyJr.h"

byte            game_slate[DIMENSION][DIMENSION];

byte            last_button_state;
byte            button_a;
byte            button_b;
byte            button_up;
byte            button_down;
byte            button_left;
byte            button_right;

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
    ,                           // Extra bright cursor position color (not 
                                // 
    // 
    // 
    // 
    // white).
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
    i = 0;
    while (i < 8) {
        j = 0;
        while (j < 8) {
            game_slate[i][j] = 0;
            j++;
        }
        i++;
    }
}


void
DisplaySlate(void)
{
    byte            j = 0;
    while (j < 8) {
        SetPixelColour(j, 7, colour_table[game_slate[j][7]]);
        SetPixelColour(j, 6, colour_table[game_slate[j][6]]);
        SetPixelColour(j, 5, colour_table[game_slate[j][5]]);
        SetPixelColour(j, 4, colour_table[game_slate[j][4]]);
        SetPixelColour(j, 3, colour_table[game_slate[j][3]]);
        SetPixelColour(j, 2, colour_table[game_slate[j][2]]);
        SetPixelColour(j, 1, colour_table[game_slate[j][1]]);
        SetPixelColour(j, 0, colour_table[game_slate[j][0]]);
        j++;
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

void
SoundOn(void)
{
    SoundState(1);
}

void
SoundOff(void)
{
    SoundState(0);
}

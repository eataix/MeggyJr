#include "MeggyJr.h"

#define DIMENSION 8

byte            GameSlate[DIMENSION][DIMENSION];

byte            lastButtonState;
byte            Button_A;
byte            Button_B;
byte            Button_Up;
byte            Button_Down;
byte            Button_Left;
byte            Button_Right;

#define MeggyCursorColor   15,15,15     // You can define color constants
                                        // like this.

// Color lookup Table
byte            ColorTable[26][3] = {
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


// Assign those colors names that we can use:
enum colors {
    Dark, Red, Orange, Yellow, Green, Blue, Violet, White,
    DimRed, DimOrange, DimYellow, DimGreen, DimAqua, DimBlue, DimViolet,
    FullOn,
    CustomColor0, CustomColor1, CustomColor2, CustomColor3, CustomColor4,
    CustomColor5, CustomColor6, CustomColor7, CustomColor8, CustomColor9
};


void
CheckButtonDown(void)
{
    byte            i = getButtons();

    Button_B = (i & 1);
    Button_A = (i & 2);
    Button_Up = (i & 4);
    Button_Down = (i & 8);
    Button_Left = (i & 16);
    Button_Right = (i & 32);

    lastButtonState = i;
}

void
CheckButtonsPress(void)
{
    byte            j;
    byte            i = getButtons();

    Button_B = (i & 1);
    Button_A = (i & 2);
    Button_Up = (i & 4);
    Button_Down = (i & 8);
    Button_Left = (i & 16);
    Button_Right = (i & 32);

    lastButtonState = i;
}

void
SetLeds(byte InputLEDs)
{
    leds = InputLEDs;
}


void
SetAuxLEDsBinary(byte n)
{
    n = (n & 240) >> 4 | (n & 15) << 4;
    n = (n & 204) >> 2 | (n & 51) << 2;
    leds = (n & 170) >> 1 | (n & 85) << 1;
}

void
DrawPx(byte x, byte y, byte colour)
{
    GameSlate[x][y] = colour;
}

// Same as above, except checks to see if pixel is onscreen
// This function is new as of v 1.4
void
SafeDrawPx(byte xin, byte yin, byte color)
{
    if ((xin >= 0) && (xin <= 7) && (yin >= 0) && (yin <= 7))
        GameSlate[xin][yin] = color;
}

// function to read color of pixel at position (x,y):
byte
ReadPx(byte xin, byte yin)
{
    return GameSlate[xin][yin];
}


// Empty the Game Slate:
void
ClearSlate(void)
{
    byte            i;
    byte            j;
    i = 0;
    while (i < 8) {
        j = 0;
        while (j < 8) {
            GameSlate[i][j] = 0;
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
        setPixClr(j, 7, ColorTable[GameSlate[j][7]]);
        setPixClr(j, 6, ColorTable[GameSlate[j][6]]);
        setPixClr(j, 5, ColorTable[GameSlate[j][5]]);
        setPixClr(j, 4, ColorTable[GameSlate[j][4]]);
        setPixClr(j, 3, ColorTable[GameSlate[j][3]]);
        setPixClr(j, 2, ColorTable[GameSlate[j][2]]);
        setPixClr(j, 1, ColorTable[GameSlate[j][1]]);
        setPixClr(j, 0, ColorTable[GameSlate[j][0]]);
        j++;
    }
}

void
SimpleInit(void)
{
    init();
    clear();
    lastButtonState = getButtons();
}

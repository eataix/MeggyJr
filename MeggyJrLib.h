#include "MeggyJr.h"

#define DIMENSION 8

byte            game_slate[DIMENSION][DIMENSION];

byte            last_button_state;
byte            button_a;
byte            button_b;
byte            button_up;
byte            button_down;
byte            button_left;
byte            button_right;

#define MeggyCursorColor   15,15,15     // You can define color constants
                                        // like this.

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

// Same as above, except checks to see if pixel is onscreen
// This function is new as of v 1.4
void
SafeDraw(byte x, byte y, byte color)
{
    if ((x >= 0) && (x <= 7) && (y >= 0) && (y <= 7))
        game_slate[x][y] = color;
}

// function to read color of pixel at position (x,y):
byte
ReadPixel(byte x, byte y)
{
    return game_slate[x][y];
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




#define ToneB2      64783

#define ToneC3      61157
#define ToneCs3     57724
#define ToneD3      54485
#define ToneDs3     51427
#define ToneE3      48541
#define ToneF3      45816
#define ToneFs3     43243
#define ToneG3      40816
#define ToneGs3     38526
#define ToneA3      36363
#define ToneAs3     34323
#define ToneB3      32397

#define ToneC4      30578
#define ToneCs4     28862
#define ToneD4      27242
#define ToneDs4     25713
#define ToneE4      24270
#define ToneF4      22908
#define ToneFs4     21622
#define ToneG4      20408
#define ToneGs4     19263
#define ToneA4      18182
#define ToneAs4     17161
#define ToneB4      16198

#define ToneC5      15289
#define ToneCs5     14431
#define ToneD5      13626
#define ToneDs5     12857
#define ToneE5      12135
#define ToneF5      11454
#define ToneFs5     10811
#define ToneG5      10204
#define ToneGs5     9631
#define ToneA5      9091
#define ToneAs5     8581
#define ToneB5      8099

#define ToneC6      7645
#define ToneCs6     7215
#define ToneD6      6810
#define ToneDs6     6428
#define ToneE6      6067
#define ToneF6      5727
#define ToneFs6     5405
#define ToneG6      5102
#define ToneGs6     4816
#define ToneA6      4545
#define ToneAs6     4290
#define ToneB6      4050

#define ToneC7      3822
#define ToneCs7     3608
#define ToneD7      3406
#define ToneDs7     3214
#define ToneE7      3034
#define ToneF7      2863
#define ToneFs7     2703
#define ToneG7      2551
#define ToneGs7     2408
#define ToneA7      2273
#define ToneAs7     2145
#define ToneB7      2025

#define ToneC8      1911
#define ToneCs8     1804
#define ToneD8      1703
#define ToneDs8     1607
#define ToneE8      1517
#define ToneF8      1432
#define ToneFs8     1351
#define ToneG8      1276
#define ToneGs8     1204
#define ToneA8      1136
#define ToneAs8     1073
#define ToneB8      1012

#define ToneC9      956
#define ToneCs9     902
#define ToneD9      851
#define ToneDs9     803

  /*
   * Sound output:
   * 
   * Frequency is given by the following formula:
   * 
   * f= 8 MHz/divisor, so divisor = 8 MHz/f.  (Round to nearest.) Maximum
   * divisor: 65535, so min. frequency is 122 Hz
   * 
   * Example: for 440 Hz, divisor = 18182
   * 
   */

   // "Cheater" functions
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

#define MakingSound  (TCCR1B > 0)


 // other sound functions:
void
ToneStart(unsigned int divisor, unsigned int duration_ms)
{
    StartTone(divisor, duration_ms);
}

/*
 * // Stop the note if its completion time has come. void
 * Tone_Update(void) { // Obsolete with current version of library; sounds 
 * stop automatically. // please remove this function from your program if 
 * it is there. } 
 */

void
SimpleInit(void)
{
    Init();
    ClearFrame();
    last_button_state = GetButtons();
    StartTone(0, 0);
    SoundOff();
}

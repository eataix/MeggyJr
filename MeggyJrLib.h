#ifndef _MEGGYJRLIB_H
#define _MEGGYJRLIB_H

#include "MeggyJr.h"

#define DIMENSION 8

#define MeggyCursorColor   15,15,15     // You can define color constants
                                        // like this.


// Assign those colors names that we can use:
enum colors {
    Dark, Red, Orange, Yellow, Green, Blue, Violet, White,
    DimRed, DimOrange, DimYellow, DimGreen, DimAqua, DimBlue, DimViolet,
    FullOn,
    CustomColor0, CustomColor1, CustomColor2, CustomColor3, CustomColor4,
    CustomColor5, CustomColor6, CustomColor7, CustomColor8, CustomColor9
};


void            CheckButtonDown(void);

void            CheckButtonPressed(void);

void            SetLed(byte n);

void            SetLedBinary(byte n);


void            Draw(byte x, byte y, byte colour);

void            SafeDraw(byte x, byte y, byte color);

byte            ReadPixel(byte x, byte y);

void            ClearSlate(void);

void            DisplaySlate(void);

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


void
                SoundOn(void);

void
                SoundOff(void);

#define MakingSound  (TCCR1B > 0)


void            ToneStart(unsigned int divisor, unsigned int duration_ms);

void            SimpleInit(void);

#endif

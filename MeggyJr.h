#ifndef _MEGGYJR_H
#define _MEGGYJR_H

#include <inttypes.h>
#include <avr/io.h>
#define byte uint8_t
#define DISP_BUFFER_SIZE 192
#define MAX_BT 15
#define FPS 120
#define F_CPU 16000000UL

// Predefined Colors: 
#define MeggyDark      0,  0,   0
#define MeggyRed       6,  0,   0
#define MeggyOrange   12,  5,   0
#define MeggyYellow    7,  10,  0
#define MeggyGreen     0,  15,  0
#define MeggyBlue      0,   0,  5
#define MeggyViolet    8,   0,  4
#define MeggyWhite     3,  15,  2

#define MeggyDimRed    1,  0,   0
#define MeggyDimGreen  0,  1,   0
#define MeggyDimBlue   0,  0,   1
#define MeggyDimOrange 1,  1,   0
#define MeggydimYellow 1,  1,   0
#define MeggydimAqua   0,  3,   1
#define MeggydimViolet 2,  0,   1

byte            frame[DISP_BUFFER_SIZE];
byte            leds;

void            ClearFrame(void);

void            Init(void);

void            SetPixelColour(byte x, byte y, byte * rgb);

byte            GetPixelRed(byte x, byte y);

byte            GetPixelGreen(byte x, byte y);

byte            GetPixelBlue(byte x, byte y);

void            ClearPixel(byte x, byte y);

byte            GetButtons(void);

void            StartTone(unsigned int tone, unsigned int duration);

void            SoundState(byte t);

void            Delay(uint16_t ms);

#endif

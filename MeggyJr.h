#include <inttypes.h>
#include <avr/io.h>
#define byte uint8_t
#define DISP_BUFFER_SIZE 192
#define MAX_BT 15
#define FPS 120

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

byte frame[DISP_BUFFER_SIZE];
byte leds;

void clear(void);

void init(void);

void setPixClr(byte x, byte y, byte *rgb);

byte getPxR(byte x, byte y);

byte getPxG(byte x, byte y);

byte getPxB(byte x, byte y);

byte getButtons(void);

void delay(uint16_t ms);

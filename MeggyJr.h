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

extern byte     leds;

void            meggyjr_init(void);

void            meggyjr_clear_frame(void);

void            meggyjr_set_pixel_color(byte x, byte y, byte * rgb);

byte            meggyjr_get_pixel_red(byte x, byte y);

byte            meggyjr_get_pixel_green(byte x, byte y);

byte            meggyjr_get_pixel_blue(byte x, byte y);

void            meggyjr_clear_pixel(byte x, byte y);

byte            meggyjr_get_button(void);

void            meggyjr_start_tone(unsigned int tone,
                                   unsigned int duration);

void            meggyjr_set_sound_state(byte t);

void            meggyjr_delay(uint16_t ms);

#endif

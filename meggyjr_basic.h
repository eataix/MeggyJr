/*-
 *  Copyright (c) 2010 Windell H. Oskay
 *  Copyright (c) 2012 Meitian Huang <_@freeaddr.info>
 *  All rights reserved.
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _MEGGYJR_H
#define _MEGGYJR_H

#include <inttypes.h>
#include <avr/io.h>
#define byte uint8_t

#define DISP_BUFFER_SIZE 192
#define MAX_BT 15
#define FPS 120
#define F_CPU 16000000UL

/*
 * For the MeggyJr I have, the second colour table gives the desirable
 * output.
 */
#define UseNewColors

/*
 * Predefined Colors: 
 */
#ifdef UseColorMap3

#define MeggyDark      0,  0,   0
#define MeggyRed       12,  0,   0
#define MeggyOrange   12,  1,   0
#define MeggyYellow    10,  4,  0
#define MeggyGreen     0,  5,  0
#define MeggyBlue      0,   0,  5
#define MeggyViolet    8,   0,  4
#define MeggyWhite     14,  4,  2

#define MeggyDimRed    4,  0,   0
#define MeggyDimGreen  0,  1,   0
#define MeggyDimBlue   0,  0,   1
#define MeggyDimOrange 6,  1,   0
#define MeggydimYellow 4,  1,   0
#define MeggydimAqua   0,  3,   1
#define MeggydimViolet 2,  0,   1

#else

#ifdef UseNewColors

#define MeggyDark      0,  0,   0
#define MeggyRed       6,  0,   0
#define MeggyOrange   12,  1,   0
#define MeggyYellow    10,  4,  0
#define MeggyGreen     0,  6,  0
#define MeggyBlue      0,   0,  5
#define MeggyViolet    8,   0,  4
#define MeggyWhite     7,  4,  2

#define MeggyDimRed    2,  0,   0
#define MeggyDimGreen  0,  1,   0
#define MeggyDimBlue   0,  0,   1
#define MeggyDimOrange 5,  1,   0
#define MeggydimYellow 3,  1,   0
#define MeggydimAqua   0,  3,   1
#define MeggydimViolet 2,  0,   1

#else

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

#endif

#endif

extern volatile byte leds;

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

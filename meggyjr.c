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

#include "meggyjr.h"
#include "meggyjr_basic.h"

volatile uint8_t meggyjr_button_a;
volatile uint8_t meggyjr_button_b;
volatile uint8_t meggyjr_button_up;
volatile uint8_t meggyjr_button_down;
volatile uint8_t meggyjr_button_left;
volatile uint8_t meggyjr_button_right;

static volatile uint8_t meggyjr_game_slate[DIMENSION][DIMENSION];
static volatile uint8_t last_button_state;

// Color lookup Table
static uint8_t  meggyjr_colour_table[26][3] = {
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
meggyjr_check_button_down(void)
{
    uint8_t         i;
    i = meggyjr_get_button();

    meggyjr_button_b = (i & 1);
    meggyjr_button_a = (i & 2);
    meggyjr_button_up = (i & 4);
    meggyjr_button_down = (i & 8);
    meggyjr_button_left = (i & 16);
    meggyjr_button_right = (i & 32);

    last_button_state = i;
}

void
meggyjr_check_button_pressed(void)
{
    uint8_t         i,
                    j;

    i = meggyjr_get_button();
    j = i & ~(last_button_state);

    meggyjr_button_b = (j & 1);
    meggyjr_button_a = (j & 2);
    meggyjr_button_up = (j & 4);
    meggyjr_button_down = (j & 8);
    meggyjr_button_left = (j & 16);
    meggyjr_button_right = (j & 32);

    last_button_state = i;
}

inline void
meggyjr_set_led(uint8_t n)
{
    leds = n;
}

inline void
meggyjr_set_led_binary(uint8_t n)
{
    n = (n & 240) >> 4 | (n & 15) << 4;
    n = (n & 204) >> 2 | (n & 51) << 2;
    leds = (n & 170) >> 1 | (n & 85) << 1;
}

inline void
meggyjr_draw(uint8_t x, uint8_t y, uint8_t colour)
{
    meggyjr_game_slate[x][y] = colour;
}

inline          uint8_t
meggyjr_read_pixel(uint8_t x, uint8_t y)
{
    return meggyjr_game_slate[x][y];
}

void
meggyjr_clear_slate(void)
{
    uint8_t         i;
    uint8_t         j;

    for (i = 0; i < 8; ++i) {
        for (j = 0; j < 8; ++j) {
            meggyjr_game_slate[i][j] = 0;
        }
    }
}

void
meggyjr_display_slate(void)
{
    uint8_t         i,
                    j;

    for (i = 0; i < 8; ++i) {
        for (j = 0; j < 8; ++j) {
            meggyjr_set_pixel_color(i, j,
                                    meggyjr_colour_table
                                    [meggyjr_game_slate[i][j]]);
        }
    }
}

inline void
meggyjr_tone_start(unsigned int divisor, unsigned int duration_ms)
{
    meggyjr_start_tone(divisor, duration_ms);
}

inline void
meggyjr_sound_enable(void)
{
    meggyjr_set_sound_state(1);
}

inline void
meggyjr_sound_disable(void)
{
    meggyjr_set_sound_state(0);
}

void
meggyjr_setup(void)
{
    meggyjr_init();
    meggyjr_clear_frame();
    last_button_state = meggyjr_get_button();
    meggyjr_sound_disable();
}

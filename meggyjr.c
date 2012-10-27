#include "meggyjr.h"
#include "meggyjr_basic.h"

volatile byte   meggyjr_button_a;
volatile byte   meggyjr_button_b;
volatile byte   meggyjr_button_up;
volatile byte   meggyjr_button_down;
volatile byte   meggyjr_button_left;
volatile byte   meggyjr_button_right;

static volatile byte meggyjr_game_slate[DIMENSION][DIMENSION];
static volatile byte last_button_state;


// Color lookup Table
static byte     meggyjr_colour_table[26][3] = {
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
    byte            i;
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
    byte            i,
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

void
meggyjr_set_led(byte n)
{
    leds = n;
}

void
meggyjr_set_led_binary(byte n)
{
    n = (n & 240) >> 4 | (n & 15) << 4;
    n = (n & 204) >> 2 | (n & 51) << 2;
    leds = (n & 170) >> 1 | (n & 85) << 1;
}

void
meggyjr_draw(byte x, byte y, byte colour)
{
    meggyjr_game_slate[x][y] = colour;
}

void
meggyjr_safe_draw(byte x, byte y, byte color)
{
    if ((x >= 0) && (x <= 7) && (y >= 0) && (y <= 7))
        meggyjr_game_slate[x][y] = color;
}

byte
meggyjr_read_pixel(byte x, byte y)
{
    return meggyjr_game_slate[x][y];
}

void
meggyjr_clear_slate(void)
{
    byte            i;
    byte            j;

    for (i = 0; i < 8; ++i) {
        for (j = 0; j < 8; ++j) {
            meggyjr_game_slate[i][j] = 0;
        }
    }
}

void
meggyjr_display_slate(void)
{
    byte            i,
                    j;

    for (i = 0; i < 8; ++i) {
        for (j = 0; j < 8; ++j) {
            meggyjr_set_pixel_color(i, j,
                                    meggyjr_colour_table[meggyjr_game_slate
                                                         [i][j]]);
        }
    }
}

inline void
meggyjr_tone_start(unsigned int divisor, unsigned int duration_ms)
{
    meggyjr_start_tone(divisor, duration_ms);
}

void
meggyjr_sound_enable(void)
{
    meggyjr_set_sound_state(1);
}

void
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
    // StartTone(0, 0);
    meggyjr_sound_disable();
}

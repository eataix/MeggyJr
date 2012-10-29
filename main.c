/*-
 * Copyright (c) 2010       Justin Shaw <wyojustin@gmail.com>
 * Copyright (c) 2012       Meitian Huang <_@freeaddr.info>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <avr_thread.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>

#include "meggyjr.h"

#define MAX_SCORE 4096
#define ABS(a) (((a) < 0) ? -(a) : (a))

/*
 * Variables
 */

/*
 * Location of the cursor
 */
uint8_t         xc,
                yc;

uint8_t         player_turn,    /* 1 if it is player's turn */
                sound_enabled;  /* 1 if the sound is enabled */

uint8_t         player_colors[] = { Red, Yellow, Dark };

uint8_t         game_over;

uint8_t         cols[7],
                rows[7];

int             tune_win[] =
    { ToneG4, ToneA5, ToneB5, ToneA5, ToneC6, ToneD6, 0 };

int             tune_lose[] =
    { ToneG4, ToneE4, ToneF4, ToneE4, ToneD4, ToneC4, ToneC4, 0 };

uint8_t         tone_current;

struct avr_thread_mutex *mutex,
                        *mutex_save;

volatile uint8_t button_a;
volatile uint8_t button_up;
volatile uint8_t button_down;
volatile uint8_t button_left;
volatile uint8_t button_right;

struct avr_thread
               *main_thread,
               *key_thread,
               *led_thread,
               *saving_thread;

/*
 * Prototypes
 */
void            loop(void);

void            draw_splash(void);

void            flash_screen(int n, int ms);

void            clear_board(void);

void            draw_board(void);

void            swipe_image(uint8_t * new_image);

void            heavy(void);

void            next_player(void);

uint8_t         check_three(uint8_t i, uint8_t j);

void            flash_three(void);

void            player_move(void);

void            computer_move(void);

int             calculate_score(uint8_t col);

void            button_buffer_entry(void);

void            led_entry(void);

void            save_game(void);

void
button_buffer_entry(void)
{
    while (1) {
        avr_thread_mutex_lock(mutex);
        meggyjr_check_button_pressed();
        if (meggyjr_button_a) {
            avr_thread_resume(led_thread);
            button_a = 1;
        }
        if (meggyjr_button_b) {
            avr_thread_pause(led_thread);
            meggyjr_button_b = 0;
        }
        if (meggyjr_button_up) {
            avr_thread_cancel(led_thread);
            button_up = 1;
        }
        if (meggyjr_button_down) {
            button_down = 1;
        }
        if (meggyjr_button_left) {
            button_left = 1;
        }
        if (meggyjr_button_right) {
            button_right = 1;
        }
        avr_thread_mutex_unlock(mutex);
        avr_thread_yield();
    }
}

void
led_entry(void)
{
    while (1) {
        if (player_turn) {
            meggyjr_set_led_binary(0b01010101);
        } else {
            meggyjr_set_led_binary(0b10101010);
        }
        avr_thread_sleep(3);
    }
}

void
save_entry(void)
{
    while (1) {
        avr_thread_mutex_lock(mutex_save);
        save_game();
        avr_thread_mutex_unlock(mutex_save);
        avr_thread_yield();
    }
}

uint8_t         key_stack[50],
                led_stack[50],
                save_stack[300];

int
main(void)
{
    meggyjr_setup();
    meggyjr_clear_slate();
    main_thread = avr_thread_init(300, atp_normal);

    button_a = 0;
    button_up = 0;
    button_down = 0;
    button_left = 0;
    button_right = 0;

    mutex = avr_thread_mutex_init();
    mutex_save = avr_thread_mutex_init();

    key_thread = avr_thread_create(button_buffer_entry, key_stack,
                                   sizeof key_stack, atp_normal);

    led_thread = avr_thread_create(led_entry, led_stack,
                                   sizeof led_stack, atp_normal);

    saving_thread = avr_thread_create(save_entry, save_stack,
                                      sizeof save_stack, atp_normal);

    restore_game();

    while (1) {
        loop();
    }
}

void
restore_game(void)
{
    uint8_t         board[8][8];

    game_over = eeprom_read_byte((uint8_t*)23);

    if (game_over) {
        xc = 6;
        yc = 6;

        player_turn = 1;
        draw_splash();
        draw_board();
        game_over = 0;
        tone_current = 0;
        button_a = 0;
        sound_enabled = 1;
    } else {

        eeprom_read_block((void *)&board, (const void*)40, 64);
        xc = eeprom_read_byte((uint8_t *)20);
        yc = eeprom_read_byte((uint8_t *)21);
        player_turn = eeprom_read_byte((uint8_t*)22);
        tone_current = eeprom_read_byte((uint8_t*)24);
        sound_enabled = eeprom_read_byte((uint8_t*)25);

        swipe_image(board);
    }
}

void
loop(void)
{
    avr_thread_mutex_lock(mutex);

    if (button_a) {
        xc = 6;
        yc = 6;

        player_turn = 1;
        draw_splash();
        draw_board();
        game_over = 0;
        tone_current = 0;
        button_a = 0;
        sound_enabled = 1;
    }

    if (button_up) {
        sound_enabled = !sound_enabled;
        if (sound_enabled) {
            meggyjr_tone_start(ToneC5, 30);
        }
        button_up = 0;
    }

    avr_thread_mutex_unlock(mutex);
    if (game_over) {
        meggyjr_draw(xc, yc, Dark);
        flash_three();
        if (player_turn == 1) {
            if (sound_enabled && tune_win[tone_current] != 0) {
                meggyjr_tone_start(tune_win[tone_current], 100);
                ++tone_current;
            }
        } else {
            if (sound_enabled && tune_lose[tone_current] != 0) {
                meggyjr_tone_start(tune_lose[tone_current], 100);
                ++tone_current;
            }
        }
    } else if (player_turn == 1) {
        player_move();
    } else {
        computer_move();
    }

    avr_thread_yield();
}

void
draw_splash(void)
{
    int             i,
                    j;

    for (j = 0; j < 8; ++j) {
        for (i = 0; i < 8; ++i) {
            meggyjr_draw(i, j, player_colors[0]);
            meggyjr_draw(7 - i, 7 - j, player_colors[1]);
            meggyjr_display_slate();
            avr_thread_sleep(1);
        }
    }

    avr_thread_sleep(1);
    flash_screen(4, 3);
}

void
flash_screen(int n, int ms)
{
    uint8_t         count,
                    i,
                    j;
    uint8_t         save_screen[64];

    for (count = 0; count < n; ++count) {
        for (j = 0; j < 8; ++j) {
            for (i = 0; i < 8; ++i) {
                save_screen[8 * j + i] = meggyjr_read_pixel(i, j);
                meggyjr_draw(i, j, Dark);
            }
        }
        meggyjr_display_slate();
        avr_thread_sleep(ms);
        for (j = 0; j < 8; ++j) {
            for (i = 0; i < 8; ++i) {
                meggyjr_draw(i, j, save_screen[8 * j + i]);
            }
        }
        meggyjr_display_slate();
        avr_thread_sleep(ms);
    }
}

void
clear_board(void)
{
    uint8_t         i,
                    j;

    for (j = 7; j > 0; --j) {
        for (i = 0; i < 8; ++i) {
            meggyjr_draw(i, j, Dark);
        }
    }

    if (sound_enabled) {
        meggyjr_tone_start(ToneF5 + (((ToneA5 - ToneF5) / 7) * j),
                           30);
    }

    meggyjr_display_slate();
    avr_thread_sleep(1);
}

void
save_game(void)
{   
    uint8_t         i,
                    j;
    uint8_t         board[8][8];

    for (i = 0; i < 8; ++i) {
        for (j = 0; j < 8; ++j) {
            board[j][i] = meggyjr_read_pixel(i, j);
        }
    }
    eeprom_update_block((const void*)board, (void*)40, 64);

    eeprom_update_byte((uint8_t*)20, xc);
    eeprom_update_byte((uint8_t*)21, yc);
    eeprom_update_byte((uint8_t*)22, player_turn);
    eeprom_update_byte((uint8_t*)23, game_over);
    eeprom_update_byte((uint8_t*)24, tone_current);
    eeprom_update_byte((uint8_t*)25, sound_enabled);
}


void
draw_board(void)
{
    uint8_t         i;
    uint8_t         board[64];

    for (i = 0; i < 64; ++i) {
        board[i] = Dark;
    }

    for (i = 0; i < 8; ++i) {
        board[8 * 7 + i] = White;
        board[8 * i] = White;
        board[8 * i + 7] = White;
    }

    swipe_image(board);
}

void
swipe_image(uint8_t * new_image)
{
    uint8_t         i,
                    j;
    int             wait = 1;
    for (j = 0; j < 8; ++j) {
        for (i = 0; i < 8; ++i) {
            meggyjr_draw(i, j, new_image[8 * j + i]);
        }
        meggyjr_display_slate();
        avr_thread_sleep(wait);
    }
}

void
heavy()
{
    int             row,
                    wait;
    avr_thread_mutex_lock(mutex_save);
    wait = 4;

    row = yc;

    if (meggyjr_read_pixel(xc, 5) == Dark) {
        row = 5;
        while (meggyjr_read_pixel(xc, row) == Dark && row >= 0) {
            meggyjr_draw(xc, row + 1, Dark);
            meggyjr_draw(xc, row, player_colors[player_turn]);
            meggyjr_display_slate();
            avr_thread_sleep(wait);
            --row;
        }
        row++;
        game_over = check_three(xc, row);

        if (game_over) {
            tone_current = 0;
        }
        next_player();
    }
    avr_thread_mutex_unlock(mutex_save);
}

inline void
next_player(void)
{
    if (player_turn == 0) {
        player_turn = 1;
    } else {
        player_turn = 0;
    }
}

uint8_t
check_three(uint8_t i, uint8_t j)
{
    uint8_t         in_a_row;
    uint8_t         color = meggyjr_read_pixel(i, j);

    int             row,
                    col;

    cols[0] = i;
    rows[0] = j;
    in_a_row = 1;
    for (row = j - 1; row >= 0; --row) {
        if (meggyjr_read_pixel(i, row) == color) {
            cols[in_a_row] = i;
            rows[in_a_row] = row;
            ++in_a_row;
        } else {
            break;
        }
    }
    if (in_a_row >= 3) {
        return 1;
    }

    cols[0] = i;
    rows[0] = j;
    in_a_row = 1;
    for (col = i - 1; col >= 0; --col) {
        if (meggyjr_read_pixel(col, j) == color) {
            cols[in_a_row] = col;
            rows[in_a_row] = j;
            ++in_a_row;
        } else {
            break;
        }
    }

    for (col = i + 1; col < 8; ++col) {
        if (meggyjr_read_pixel(col, j) == color) {
            cols[in_a_row] = col;
            rows[in_a_row] = j;
            ++in_a_row;
        } else {
            break;
        }
    }
    if (in_a_row >= 3) {
        return 1;
    }

    cols[0] = i;
    rows[0] = j;
    in_a_row = 1;
    col = i - 1;
    row = j - 1;
    while (col >= 1 && row >= 0) {
        if (meggyjr_read_pixel(col, row) == color) {
            cols[in_a_row] = col;
            rows[in_a_row] = row;
            ++in_a_row;
        } else {
            break;
        }
        --col;
        --row;
    }

    col = i + 1;
    row = j + 1;
    while (col <= 7 && row <= 6) {
        if (meggyjr_read_pixel(col, row) == color) {
            cols[in_a_row] = col;
            rows[in_a_row] = row;
            ++in_a_row;
        } else {
            break;
        }
        ++col;
        ++row;
    }

    if (in_a_row >= 3) {
        return 1;
    }

    cols[0] = i;
    rows[0] = j;
    in_a_row = 1;

    col = i - 1;
    row = j + 1;
    while (col >= 1 && col <= 6) {
        if (meggyjr_read_pixel(col, row) == color) {
            cols[in_a_row] = col;
            rows[in_a_row] = row;
            ++in_a_row;
        } else {
            break;
        }
        --col;
        ++row;
    }

    col = i + 1;
    row = j - 1;
    while (col <= 7 && row >= 0) {
        if (meggyjr_read_pixel(col, row) == color) {
            cols[in_a_row] = col;
            rows[in_a_row] = row;
            ++in_a_row;
        } else {
            break;
        }
        ++col;
        --row;
    }

    if (in_a_row >= 3) {
        return 1;
    }

    return 0;
}

void
flash_three(void)
{
    uint8_t         save[3];
    uint8_t         i;

    for (i = 0; i < 3; ++i) {
        save[i] = meggyjr_read_pixel(cols[i], rows[i]);
        meggyjr_draw(cols[i], rows[i], player_colors[2]);
    }
    meggyjr_display_slate();
    avr_thread_sleep(1);

    for (i = 0; i < 3; ++i) {
        meggyjr_draw(cols[i], rows[i], save[i]);
    }
    meggyjr_display_slate();
    avr_thread_sleep(3);
}

void
player_move(void)
{
    avr_thread_mutex_lock(mutex);

    if (button_down) {
        heavy();
        if (sound_enabled) {
            meggyjr_tone_start(ToneD5, 20);
        }
        button_down = 0;
    }

    if (button_right) {
        if (xc < 6) {
            meggyjr_draw(xc, yc, Dark);
            xc = (xc + 1) % 8;
            if (sound_enabled) {
                meggyjr_tone_start(ToneD5, 20);
            }
        }
        button_right = 0;
    }

    if (button_left) {
        if (xc > 1) {
            meggyjr_draw(xc, yc, Dark);
            xc = (xc - 1) % 8;
            if (sound_enabled) {
                meggyjr_tone_start(ToneC5, 20);
            }
        }
        button_left = 0;
    }

    avr_thread_mutex_unlock(mutex);

    meggyjr_draw(xc, yc, player_colors[player_turn]);
    meggyjr_display_slate();
    avr_thread_sleep(1);
}

void
computer_move(void)
{
    int             scores[8];
    int             max_score;

    for (; xc > 1; --xc) {
        meggyjr_draw(xc, yc, Dark);
        meggyjr_draw(xc - 1, yc, player_colors[player_turn]);
        meggyjr_display_slate();
        avr_thread_sleep(3);
    }
    max_score = -1;
    for (; xc < 6; ++xc) {
        scores[xc] = calculate_score(xc);
        if (scores[xc] > -1) {
            scores[xc] += 4 - ABS(4 - xc);
        }
        if (scores[xc] > max_score) {
            max_score = scores[xc];
            if (max_score == MAX_SCORE + 4 - ABS(4 - xc)) {
                break;
            }
        }
        meggyjr_draw(xc, yc, Dark);
        meggyjr_draw(xc + 1, yc, player_colors[player_turn]);
        meggyjr_display_slate();
    }

    meggyjr_draw(xc, yc, player_colors[player_turn]);

    for (; xc > 0; --xc) {
        if (scores[xc] == max_score) {
            break;
        }
        meggyjr_draw(xc, yc, Dark);
        meggyjr_draw(xc - 1, yc, player_colors[player_turn]);
        meggyjr_display_slate();
        avr_thread_sleep(3);
    }

    heavy();
}

int
calculate_score(uint8_t col)
{
    int             score,
                    r,
                    c;

    uint8_t         row;

    score = 0;

    if (meggyjr_read_pixel(col, 5) != Dark) {
        score = -1;
    } else {
        for (row = 0; row <= 6; ++row) {
            if (meggyjr_read_pixel(col, row) == Dark) {
                break;
            }
        }
        meggyjr_draw(col, row, player_colors[player_turn]);

        if (check_three(col, row)) {
            score = MAX_SCORE;
        }
        meggyjr_draw(col, row, Dark);

        if (score < MAX_SCORE) {
            score = 0;
            for (r = row - 1; r < row + 1; ++r) {
                for (c = col - 1; c <= col + 1; ++c) {
                    if (c > 0 && c < 8) {
                        if (r >= 0 && r < 6) {
                            score += meggyjr_read_pixel(r, c);
                        }
                    }
                }
            }
        }
    }
    return score;
}

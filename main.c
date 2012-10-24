#include <meggyjr.h>
#include <avr_thread.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#define MAX_SCORE 4096
#define ABS(a) (((a) < 0) ? -(a) : (a))

byte            xc,
                yc;

unsigned long   loop_counter = 0;

uint8_t         player,
                player_start,
                sound;

byte            depth = 0;
byte            n_move;

byte            player_colors[] = { Red, Yellow, Dark };

uint8_t         game_over;

byte            cols[7],
                rows[7];

int             tune_win[] =
    { ToneG4, ToneA5, ToneB5, ToneA5, ToneC6, ToneD6, 0 };

int             tune_lose[] =
    { ToneG4, ToneE4, ToneF4, ToneE4, ToneD4, ToneC4, ToneC4, 0 };

byte            tune_note;


void            loop(void);

void            blink_player(void);

void            draw_splash(void);

void            flash_screen(int n, int ms);

void            clear_board(void);

void            draw_board(void);

void            swipe_image(byte * new_image);

void            heavy(void);

void            next_player(void);

uint8_t         check_four(byte i, byte j);

void            flash_four(void);

void            player_move(void);

void            computer_move(void);

int             get_score(byte col, byte depth);

void            update_auxleds(void);

int
main(void)
{
    struct avr_thread_context *main_thread;

    meggyjr_setup();
    meggyjr_clear_slate();
    main_thread = avr_thread_init(200, atp_noromal);
    sei();

    xc = 7;
    yc = 6;

    player_start = 0;
    player = player_start;
    draw_splash();
    draw_board();
    game_over = 0;
    depth = 4;
    n_move = 1;
    tune_note = 0;
    sound = 1;
    update_auxleds();
    while (1) {
        loop();
    }
}

void
loop(void)
{

    meggyjr_check_button_down();
    if (meggyjr_button_a) {
        player_start = !player_start;
        player = player_start;
        game_over = 0;
        clear_board();
        draw_board();
    }

    if (meggyjr_button_up) {
        sound = !sound;
        if (sound) {
            meggyjr_tone_start(ToneC5, 30);
        }
        update_auxleds();
    }

    if (game_over) {
        meggyjr_draw(xc, yc, Dark);
        flash_four();
        if (player == 1) {
            if (sound && tune_win[tune_note] != 0) {
                meggyjr_tone_start(tune_win[tune_note], 100);
                ++tune_note;
            }
        } else {
            if (sound && tune_lose[tune_note] != 0) {
                meggyjr_tone_start(tune_lose[tune_note], 100);
                ++tune_note;
            }
        }
    } else if (player == 0) {
        player_move();
    } else {
        computer_move();
    }
    ++loop_counter;
}

void
blink_player(void)
{
    if (loop_counter % 1000 < 700) {
        meggyjr_draw(xc, yc, player_colors[player]);
    } else {
        meggyjr_draw(xc, yc, Dark);
    }
}

void
draw_splash(void)
{
    int             i,
                    j,
                    d;
    d = 1;

    for (j = 0; j < 8; ++j) {
        d -= 5;
        for (i = 0; i < 8; ++i) {
            meggyjr_draw(i, j, player_colors[0]);
            meggyjr_draw(7 - i, 7 - j, player_colors[1]);
            meggyjr_display_slate();
            avr_thread_sleep(1);
        }
    }

    avr_thread_sleep(1);
    flash_screen(4, 1);
}

void
flash_screen(int n, int ms)
{
    byte            count,
                    i,
                    j;
    byte            save_screen[64];

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
    int             wait;
    wait = 1;
    byte            i,
                    j;
    for (j = 7; j > 0; --j) {
        for (i = 0; i < 8; ++i) {
            meggyjr_draw(i, j, Dark);
        }
    }
    if (sound) {
        meggyjr_tone_start(ToneF5 + (((ToneA5 - ToneF5) / 7) * j), 30);
    }

    meggyjr_display_slate();
    avr_thread_sleep(wait);
}

void
draw_board(void)
{
    byte            i;
    byte            board[64];

    for (i = 0; i < 64; ++i) {
        board[i] = Dark;
    }

    for (i = 0; i < 8; ++i) {
        board[8 * 7 + i] = White;
        board[8 * i] = White;
    }

    swipe_image(board);
}

void
swipe_image(byte * new_image)
{
    byte            i,
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
    wait = 1;

    row = yc;

    if (meggyjr_read_pixel(xc, 5) == Dark) {
        row = 5;
        while (meggyjr_read_pixel(xc, row) == Dark && row >= 0) {
            meggyjr_draw(xc, row + 1, Dark);
            meggyjr_draw(xc, row, player_colors[player]);
            meggyjr_display_slate();
            avr_thread_sleep(wait);
            --row;
        }
        row++;
        game_over = check_four(xc, row);

        if (game_over) {
            tune_note = 0;
        }
        next_player();
    }
}

void
next_player(void)
{
    ++n_move;
    if (player == 0) {
        player = 1;
    } else {
        player = 0;
    }
}

uint8_t
check_four(byte i, byte j)
{
    byte            in_a_row;
    byte            color = meggyjr_read_pixel(i, j);
    uint8_t         out = 0;

    int             row,
                    col;

    if (out == 0) {
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
        if (in_a_row > 3) {
            out = 1;
        }
    }

    if (out == 0) {
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
        if (in_a_row > 3) {
            out = 1;
        }
    }

    if (out == 0) {
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

        if (in_a_row > 3) {
            out = 1;
        }
    }

    if (out == 0) {
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

        if (in_a_row > 3) {
            out = 1;
        }
    }
    return out;
}

void
flash_four(void)
{
    byte            save[4];
    byte            i;

    for (i = 0; i < 4; ++i) {
        save[i] = meggyjr_read_pixel(cols[i], rows[i]);
        meggyjr_draw(cols[i], rows[i], player_colors[2]);
    }
    meggyjr_display_slate();
    avr_thread_sleep(1);

    for (i = 0; i < 4; ++i) {
        meggyjr_draw(cols[i], rows[i], save[i]);
    }
    meggyjr_display_slate();
    avr_thread_sleep(1);
}

void
player_move(void)
{
    if (meggyjr_button_b) {
        ++depth;
        if (depth == 6) {
            depth = 0;
        }
        update_auxleds();

        if (sound) {
            meggyjr_tone_start(ToneF5 + (((ToneA5 - ToneF5) / 6) * depth),
                               50);
        }
    }

    if (meggyjr_button_down) {
        heavy();
        if (sound) {
            meggyjr_tone_start(ToneD5, 20);
        }
    }

    if (meggyjr_button_right) {
        if (xc < 7) {
            meggyjr_draw(xc, yc, Dark);
            xc = (xc + 1) % 8;
            if (sound) {
                meggyjr_tone_start(ToneD5, 20);
            }
        }
    }

    if (meggyjr_button_left) {
        if (xc > 1) {
            meggyjr_draw(xc, yc, Dark);
            xc = (xc - 1) % 8;
            if (sound) {
                meggyjr_tone_start(ToneC5, 20);
            }
        }
    }

    blink_player();
    meggyjr_display_slate();
}

void
computer_move(void)
{
    byte            col;
    int             scores[8];
    int             max_score;

    if (n_move < 1) {
        col = 3;                // TODO
        for (; xc > 1; --xc) {
            meggyjr_draw(xc, yc, Dark);
            meggyjr_draw(xc - 1, yc, player_colors[player]);
            meggyjr_display_slate();
            avr_thread_sleep(1);
        }
        for (; xc != col; ++xc) {
            meggyjr_draw(xc, yc, Dark);
            meggyjr_draw(xc + 1, yc, player_colors[player]);
            meggyjr_display_slate();
            avr_thread_sleep(1);
        }
    } else {
        for (; xc > 1; --xc) {
            meggyjr_draw(xc, yc, Dark);
            meggyjr_draw(xc - 1, yc, player_colors[player]);
            meggyjr_display_slate();
            avr_thread_sleep(1);
        }
        max_score = -1;
        for (; xc < 8; ++xc) {
            scores[xc] += 4 - ABS(4 - xc);
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
            meggyjr_draw(xc + 1, yc, player_colors[player]);
            meggyjr_display_slate();
        }
        meggyjr_draw(xc, yc, player_colors[player]);

        for (; xc > 0; --xc) {
            if (scores[xc] == max_score) {
                break;
            }
            meggyjr_draw(xc, yc, Dark);
            meggyjr_draw(xc - 1, yc, player_colors[player]);
            meggyjr_display_slate();
            avr_thread_sleep(1);
        }
    }
    heavy();
}

int
get_score(byte col, byte depth)
{
    int             score,
                    opp_scores[8],
                    opp_max_score,
                    r,
                    c;

    byte            row,
                    low_row,
                    opp_col,
                    in_a_row;

    score = 0;

    if (meggyjr_read_pixel(col, 5) != Dark) {
        score = -1;
    } else {
        for (row = 0; row <= 6; ++row) {
            if (meggyjr_read_pixel(col, row) == Dark) {
                break;
            }
        }
        meggyjr_draw(col, row, player_colors[player]);

        if (check_four(col, row)) {
            score = MAX_SCORE;
        }
        meggyjr_draw(col, row, Dark);

        if (score < MAX_SCORE) {
            if (depth == 0) {
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
        } else {
            meggyjr_draw(col, row, player_colors[player]);
            player = !player;

            opp_max_score = -MAX_SCORE;
            for (opp_col = 1; opp_col < 8; ++opp_col) {
                opp_scores[opp_col] = get_score(opp_col, depth - 1);
                if (opp_scores[opp_col] > opp_max_score) {
                    opp_max_score = opp_scores[opp_col];
                    if (opp_max_score == MAX_SCORE) {
                        break;
                    }
                }
            }

            meggyjr_draw(col, row, Dark);
            player = !player;
            score = (MAX_SCORE - opp_max_score) / 2;
        }
    }
    return score;
}

void
update_auxleds(void)
{
    byte            i;
    i = 1 << depth;
    if (sound) {
        i += 128;
    }
    meggyjr_set_led(i);
}

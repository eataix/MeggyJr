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

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay_basic.h>

#include "avr_thread.h"
#include "meggyjr_basic.h"

#define F_CPU 16000000UL

static volatile uint8_t frame[DISP_BUFFER_SIZE];
volatile uint8_t leds;

static volatile uint8_t current_column;
static volatile uint8_t *current_column_ptr;
static volatile uint8_t current_brightness;

static volatile unsigned int tone_time_remaining;
static volatile uint8_t sound_enabled;

void
meggyjr_init(void)
{
    leds = 0;
    current_column_ptr = frame;
    current_column = 0;
    current_brightness = 0;

    PORTC = 255U;
    DDRC = 0;

    DDRD = 254U;
    PORTD = 254U;

    DDRB = 63U;
    PORTB = 255;

    tone_time_remaining = 0;

    sound_enabled = 0;

    PORTD |= 252U;
    PORTB |= 17U;

    meggyjr_clear_frame();

    TCCR2A = (1 << WGM21);
    TCCR2B = (1 << CS21);
    OCR2A = (F_CPU >> 3) / 8 / 15 / FPS;
    TIMSK2 = (1 << OCIE2A);

    sei();
}

void
meggyjr_clear_frame(void)
{
    uint8_t         i;
    // I hope this can be as fast as memset(3).
    for (i = 0; i < DISP_BUFFER_SIZE; ++i) {
        frame[i] = 0;
    }
}

void
meggyjr_set_pixel_color(uint8_t x, uint8_t y, uint8_t * rgb)
{
    uint8_t         pixelPtr;

    pixelPtr = (uint8_t) 24 *x + y;
    frame[pixelPtr] = rgb[2];
    pixelPtr += 8;
    frame[pixelPtr] = rgb[1];
    pixelPtr += 8;
    frame[pixelPtr] = rgb[0];
}

inline          uint8_t
meggyjr_get_pixel_red(uint8_t x, uint8_t y)
{
    return frame[24 * x + y + 16];
}

inline          uint8_t
meggyjr_get_pixel_green(uint8_t x, uint8_t y)
{
    return frame[24 * x + y + 8];
}

inline          uint8_t
meggyjr_get_pixel_blue(uint8_t x, uint8_t y)
{
    return frame[24 * x + y];
}

void
meggyjr_clear_pixel(uint8_t x, uint8_t y)
{
    uint8_t         pixelPtr;
    pixelPtr = 24 * x + y;
    frame[pixelPtr] = 0;
    pixelPtr += 8;
    frame[pixelPtr] = 0;
    pixelPtr += 8;
    frame[pixelPtr] = 0;
}

inline          uint8_t
meggyjr_get_button(void)
{
    return (~(PINC) & 63U);
}

inline void
meggyjr_start_tone(unsigned int tone, unsigned int duration)
{
    OCR1A = tone;
    meggyjr_set_sound_state(1);
    tone_time_remaining = duration;
}

void
meggyjr_set_sound_state(uint8_t t)
{
    if (t) {
        TCCR1A = 65;
        TCCR1B = 17;
        sound_enabled = 1;
        DDRB |= 2;
    } else {
        sound_enabled = 0;
        TCCR1A = 0;

        if (t) {
            TCCR1B = 128;
        } else {
            TCCR1B = 0;
        }

        DDRB &= 253;
        PORTB |= 2;
    }
}

static volatile uint8_t num_redraws = 0;
static volatile uint8_t *ptr;
static uint8_t  p;
static uint8_t  cb;
static uint8_t  bits;
static uint8_t  portbTemp;
static uint8_t  portdTemp;

/**
 * ISR
 *
 * Here is the ISR.
 **/
ISR(TIMER2_COMPA_vect, ISR_NAKED)
{
    __asm__("push r0");
    __asm__("in r0, __SREG__");
    __asm__("cli");
    __asm__("push r0");
    __asm__("push r1");
    __asm__("push r2");
    __asm__("push r3");
    __asm__("push r4");
    __asm__("push r5");
    __asm__("push r6");
    __asm__("push r7");
    __asm__("push r8");
    __asm__("push r9");
    __asm__("push r10");
    __asm__("push r11");
    __asm__("push r12");
    __asm__("push r13");
    __asm__("push r14");
    __asm__("push r15");
    __asm__("push r16");
    __asm__("push r17");
    __asm__("push r18");
    __asm__("push r19");
    __asm__("push r20");
    __asm__("push r21");
    __asm__("push r22");
    __asm__("push r23");
    __asm__("push r24");
    __asm__("push r25");
    __asm__("push r26");
    __asm__("push r27");
    __asm__("push r28");
    __asm__("push r29");
    __asm__("push r30");
    __asm__("push r31");
    __asm__("clr __zero_reg__");

    if (++current_brightness >= MAX_BT) {
        current_brightness = 0;
        ++current_column;
        if (current_column > 7) {
            current_column = 0;
            current_column_ptr = frame;
            /*
             * You don't pay for what you don't use!
             */
            if (avr_thread_initialised == 1) {
                ++num_redraws;
                if (num_redraws == FPS / FIRE_PER_SEC) {
                    /*
                     * In AVR, SP is directly readable and writeable.
                     * Yet, its type is trick. It is uint16_t according
                     * to the manual of AVR Libc. Thus, I need to do
                     * two conversions.
                     */
                    SP = (uint16_t) avr_thread_tick((uint8_t *) SP);
                    num_redraws = 0;
                }
            }
        } else {
            current_column_ptr += 24;   // 3 * 8
        }

        if (tone_time_remaining > 0) {
            --tone_time_remaining;
            if (tone_time_remaining == 0) {
                TCCR1A = 0;
                DDRB &= 253;
                PORTB |= 2;
                TCCR1B = 0;
            }
        }
    }

    ptr = current_column_ptr + 23;
    cb = current_brightness;

    PORTD |= 252U;
    PORTB |= 17U;

    SPCR = 80;

    if ((cb + current_column) == 0 || (cb + current_column) == 4) {
        SPDR = leds;
    } else {
        SPDR = 0;
    }

    bits = 0;

    p = *ptr--;
    if (p > cb) {
        bits |= 128;
    }
    p = *ptr--;
    if (p > cb) {
        bits |= 64;
    }
    p = *ptr--;
    if (p > cb) {
        bits |= 32;
    }
    p = *ptr--;
    if (p > cb) {
        bits |= 16;
    }
    p = *ptr--;
    if (p > cb) {
        bits |= 8;
    }
    p = *ptr--;
    if (p > cb) {
        bits |= 4;
    }
    p = *ptr--;
    if (p > cb) {
        bits |= 2;
    }
    p = *ptr--;
    if (p > cb) {
        bits |= 1;
    }

    SPDR = bits;

    bits = 0;
    p = *ptr--;
    if (p > cb) {
        bits |= 128;
    }
    p = *ptr--;
    if (p > cb) {
        bits |= 64;
    }
    p = *ptr--;
    if (p > cb) {
        bits |= 32;
    }
    p = *ptr--;
    if (p > cb) {
        bits |= 16;
    }
    p = *ptr--;
    if (p > cb) {
        bits |= 8;
    }
    p = *ptr--;
    if (p > cb) {
        bits |= 4;
    }
    p = *ptr--;
    if (p > cb) {
        bits |= 2;
    }
    p = *ptr--;
    if (p > cb) {
        bits |= 1;
    }

    while (!(SPSR && (1 << SPIF))) {
        // First Spin;
    }
    SPDR = bits;

    bits = 0;
    p = *ptr--;
    if (p > cb) {
        bits |= 128;
    }
    p = *ptr--;
    if (p > cb) {
        bits |= 64;
    }
    p = *ptr--;
    if (p > cb) {
        bits |= 32;
    }
    p = *ptr--;
    if (p > cb) {
        bits |= 16;
    }
    p = *ptr--;
    if (p > cb) {
        bits |= 8;
    }
    p = *ptr--;
    if (p > cb) {
        bits |= 4;
    }
    p = *ptr--;
    if (p > cb) {
        bits |= 2;
    }
    p = *ptr--;
    if (p > cb) {
        bits |= 1;
    }

    while (!(SPSR && (1 << SPIF))) {
        // Second Spin;
    }
    SPDR = bits;

    portbTemp = 0;
    portdTemp = 0;

    if (current_column == 0) {
        portbTemp = 239U;
    } else if (current_column == 1) {
        portbTemp = 254U;
    } else {
        portdTemp = ~(1 << (9 - current_column));
    }

    while (!(SPSR && (1 << SPIF))) {
        // Third Spin;
    }

    PORTB |= 4;

    if (current_column > 1) {
        PORTD &= portdTemp;
    } else {
        PORTB &= portbTemp;
    }

    PORTB &= 251;

    SPCR = 0;

    __asm__("pop r31");
    __asm__("pop r30");
    __asm__("pop r29");
    __asm__("pop r28");
    __asm__("pop r27");
    __asm__("pop r26");
    __asm__("pop r25");
    __asm__("pop r24");
    __asm__("pop r23");
    __asm__("pop r22");
    __asm__("pop r21");
    __asm__("pop r20");
    __asm__("pop r19");
    __asm__("pop r18");
    __asm__("pop r17");
    __asm__("pop r16");
    __asm__("pop r15");
    __asm__("pop r14");
    __asm__("pop r13");
    __asm__("pop r12");
    __asm__("pop r11");
    __asm__("pop r10");
    __asm__("pop r9");
    __asm__("pop r8");
    __asm__("pop r7");
    __asm__("pop r6");
    __asm__("pop r5");
    __asm__("pop r4");
    __asm__("pop r3");
    __asm__("pop r2");
    __asm__("pop r1");
    __asm__("pop r0");
    __asm__("out __SREG__,r0");
    __asm__("pop r0");
    __asm__("reti");
}

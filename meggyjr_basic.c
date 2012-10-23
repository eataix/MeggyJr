#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay_basic.h>

#include "avr_thread.h"
#include "meggyjr_basic.h"

#define F_CPU 16000000UL

static byte     frame[DISP_BUFFER_SIZE];
byte            leds;

static byte     current_column;
static byte    *current_column_ptr;
static byte     current_brightness;

static unsigned int tone_time_remaining;
static byte     sound_enabled;

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

    // TODO
    // sei();
}

void
meggyjr_clear_frame(void)
{
    byte            i;
    // I hope this can be as fast as memset(3).
    for (i = 0; i < DISP_BUFFER_SIZE; ++i) {
        frame[i] = 0;
    }
}

void
meggyjr_set_pixel_color(byte x, byte y, byte * rgb)
{
    byte            pixelPtr;

    pixelPtr = (byte) 24 *x + y;
    frame[pixelPtr] = rgb[2];
    pixelPtr += 8;
    frame[pixelPtr] = rgb[1];
    pixelPtr += 8;
    frame[pixelPtr] = rgb[0];
}

byte
meggyjr_get_pixel_red(byte x, byte y)
{
    return frame[24 * x + y + 16];
}

byte
meggyjr_get_pixel_green(byte x, byte y)
{
    return frame[24 * x + y + 8];
}

byte
meggyjr_get_pixel_blue(byte x, byte y)
{
    return frame[24 * x + y];
}

void
meggyjr_clear_pixel(byte x, byte y)
{
    byte            pixelPtr;
    pixelPtr = 24 * x + y;
    frame[pixelPtr] = 0;
    pixelPtr += 8;
    frame[pixelPtr] = 0;
    pixelPtr += 8;
    frame[pixelPtr] = 0;
}

byte
meggyjr_get_button(void)
{
    return (~(PINC) & 63U);
}

void
meggyjr_start_tone(unsigned int tone, unsigned int duration)
{
    OCR1A = tone;
    meggyjr_set_sound_state(1);
    tone_time_remaining = duration;
}

void
meggyjr_set_sound_state(byte t)
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

static uint8_t  num_redraws = 0;

/**
 * ISR
 *
 * Here is the ISR.
 **/
ISR(TIMER2_COMPA_vect, ISR_NAKED)
{
    uint8_t         sp;
    __asm__("push r0");
    __asm__("in r0,__SREG__");
    __asm__("push r0");
    __asm__("cli");
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
    /*
     * __asm__("in r28, 0x3d"); __asm__("in r29, 0x3e"); __asm__("sbiw
     * r28, 0x07"); __asm__("out 0x3e, r29"); __asm__("out 0x3d, r28"); 
     */
    sp = SP;

    if (++current_brightness >= MAX_BT) {
        current_brightness = 0;
        ++current_column;
        if (current_column > 7) {
            current_column = 0;
            current_column_ptr = frame;
            ++num_redraws;
            if (num_redraws == FPS / FIRE_PER_SEC) {
                num_redraws = 0;
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

    byte           *ptr = current_column_ptr + 23;
    byte            p;
    byte            cb = current_brightness;

    PORTD |= 252U;
    PORTB |= 17U;

    SPCR = 80;

    if ((cb + current_column) == 0) {
        SPDR = leds;
    } else {
        SPDR = 0;
    }

    byte            bits = 0;

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

    byte            portbTemp = 0;
    byte            portdTemp = 0;

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

  isr_done:
    /*
     * __asm__("adiw r28, 0x07"); __asm__("out 0x3e, r29"); __asm__("out
     * 0x3d, r28"); 
     */

    SP = sp;
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

// TODO
void
meggyjr_delay(uint16_t ms)
{
    uint16_t        i,
                    j;

    uint16_t        loop = F_CPU / 17000;
    for (i = 0; i < ms; i++) {
        for (j = 0; j < loop; j++) {
            // Do nothing but burning CPU cycles.
        }
    }
}

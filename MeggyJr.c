#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay_basic.h>

#define F_CPU 16000000UL

#include "MeggyJr.h"
#include "avr_thread.h"

byte            frame[DISP_BUFFER_SIZE];
byte            leds;

byte            current_column;
byte           *current_column_ptr;
byte            current_brightness;

unsigned int    tone_time_remaining;
byte            sound_enabled;

void
Init(void)
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

    ClearFrame();

    TCCR2A = (1 << WGM21);
    TCCR2B = (1 << CS21);
    OCR2A = (F_CPU >> 3) / 8 / 15 / FPS;
    TIMSK2 = (1 << OCIE2A);

    // TODO
    // sei();
}

void
ClearFrame(void)
{
    byte            i;
    // I hope this can be as fast as memset(3).
    for (i = 0; i < DISP_BUFFER_SIZE; ++i) {
        frame[i] = 0;
    }
}

void
SetPixelColour(byte x, byte y, byte * rgb)
{
    byte            pixelPtr;

    pixelPtr = 24 * x + y;
    frame[pixelPtr] = rgb[2];
    pixelPtr += 8;
    frame[pixelPtr] = rgb[1];
    pixelPtr += 8;
    frame[pixelPtr] = rgb[0];
}

byte
GetPixelRed(byte x, byte y)
{
    return frame[24 * x + y + 16];
}

byte
GetPixelGreen(byte x, byte y)
{
    return frame[24 * x + y + 8];
}

byte
GetPixelBlue(byte x, byte y)
{
    return frame[24 * x + y];
}

void
ClearPixel(byte x, byte y)
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
GetButtons(void)
{
    return (~(PINC) & 63U);
}

void
StartTone(unsigned int tone, unsigned int duration)
{
    OCR1A = tone;
    SoundState(1);
    tone_time_remaining = duration;
}

void
SoundState(byte t)
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

/**
 * ISR
 *
 * Here is the ISR.
 **/
SIGNAL(TIMER2_COMPA_vect)
{
    cli();

    if (++current_brightness >= MAX_BT) {
        current_brightness = 0;
        ++current_column;
        if (current_column > 7) {
            current_column = 0;
            current_column_ptr = frame;
            avr_thread_tick();
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
        // Spin;
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
        // Spin;
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
        // Spin;
    }

    PORTB |= 4;

    if (current_column > 1) {
        PORTD &= portdTemp;
    } else {
        PORTB &= portbTemp;
    }

    PORTB &= 251;

    SPCR = 0;

    // TODO
    sei();
}

// TODO
void
Delay(uint16_t ms)
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

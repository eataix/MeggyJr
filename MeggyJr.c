#include <avr/io.h>
#include <avr/interrupt.h>

#include "Meg.h"

byte            frame[DISP_BUFFER_SIZE];
byte            leds;

byte            current_column;
byte           *current_column_ptr;
byte            current_brightness;

void
clear(void)
{
    byte            i;
    for (i = 0; i < DISP_BUFFER_SIZE; ++i) {
        frame[i] = 0;
    }
}

void
init(void)
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

    PORTD |= 252U;
    PORTB |= 17U;
    clear();

    TCCR2A = (1 << WGM21);
    TCCR2B = (1 << CS21);
    OCR2A = (F_CPU >> 3) / 8 / 15 / FPS;
    TIMSK2 = (1 << OCIE2A);

    sei();
}

void
setPixClr(byte x, byte y, byte * rgb)
{
    byte            pixelPtr = 24 * x + y;
    frame[pixelPtr] = rgb[2];
    pixelPtr += 8;
    frame[pixelPtr] = rgb[1];
    pixelPtr += 8;
    frame[pixelPtr] = rgb[0];
}

byte
getPxR(byte x, byte y)
{
    return frame[24 * x + y + 16];
}

byte
getPxG(byte x, byte y)
{
    return frame[24 * x + y + 8];
}

byte
getPxB(byte x, byte y)
{
    return frame[24 * x + y];
}

void
clearPixel(byte x, byte y)
{
    byte            pixelPtr = 24 * x + y;
    frame[pixelPtr] = 0;
    pixelPtr += 8;
    frame[pixelPtr] = 0;
    pixelPtr += 8;
    frame[pixelPtr] = 0;
}

byte
getButtons(void)
{
    return (~(PINC) & 63U);
}

SIGNAL(TIMER2_COMPA_vect)
{
    if (++current_brightness >= MAX_BT) {
        current_brightness = 0;
        if (++current_column > 7) {
            current_column = 0;
            current_column_ptr = frame;
        } else {
            current_column_ptr += 24;
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
    }

    PORTB |= 4;

    if (current_column > 1) {
        PORTD &= portdTemp;
    } else {
        PORTB &= portbTemp;
    }

    PORTB &= 251;

    SPCR = 0;
}

void
delay(uint16_t ms)
{
    uint16_t        i,
                    j;
    uint16_t        loop = F_CPU / 17000;

    for (i = 0; i < ms; i++) {
        for (j = 0; j < loop; j++);
    }
}

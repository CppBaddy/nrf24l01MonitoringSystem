/*
 * ws2811.c
 *
 *  Created on: 2020-09-03
 *      Author: yulay
 */

#include <avr/io.h>
#include <util/delay_basic.h>
#include <avr/interrupt.h>

#include "ws2811.h"

#include "Color24.h"
#include "PortConfig.h"

/**
 * This function sends the RGB-data in an array of rgb structs through
 * the given io-pin.
 * The port is determined by the macro WS2811_PORT, but the actual pin to
 * be used is an argument to this function. This allows a single instance of this function
 * to control up to 8 separate channels.
 */
void WS2811_Send(const void* values, uint16_t array_size)
{
    cli();

    uint8_t low_val = WS2811_PORT & (~_BV(WS2811_DRIVER));
    uint8_t high_val = WS2811_PORT | _BV(WS2811_DRIVER);
    uint16_t size = array_size * sizeof(Color24); // size in bytes


    // reset the controllers by pulling the data line low
    uint8_t bitcount = 7;
    WS2811_PORT &= ~_BV(WS2811_DRIVER);
    _delay_loop_1(107); // at 3 clocks per iteration, this is 320 ticks or 40us at 8Mhz

    // The labels in this piece of assembly code aren't very explanatory. The real documentation
    // of this code can be found in the spreadsheet ws2811@8Mhz.ods
    // A hint if you still want to follow the code below:
    // The code will send bits from most significant to least significant (bits 7 to 0) and consists
    // of two variants of the main loop:
    // 1) The code for a regular bit (i.e. bits 7-1) starts at label s00 with the current bit value
    // already in the carry flag and it jumps halfway back to label cont06.
    // 2) For a part of bit 1 and all of bit 0, the code falls through (after the skip03 label)
    // where the code needs to fork in a "transmit 0" and "transmit 1" branch. This is because
    // there's extra work to be done (loading the next byte), which needs to be carefully placed
    // in the time between toggling the output pins.
    //
    // The two-digit suffix of labels shows the "phase" of the signal at the time
    // of the execution, 00 being the first clock tick of the bit and 09 being the last.
    __asm__ __volatile__(
            "start:  LDI %[bits], 7                          \n" // start code, load bit count
            "        LD __tmp_reg__, %a[dataptr]+            \n" // fetch first byte
            "cont06: NOP                                     \n"
            "cont07: NOP                                     \n"
            "        OUT %[portout], %[downreg]              \n" // Force line down, even if it already was down
            "cont09: LSL __tmp_reg__                         \n" // Load next bit into carry flag.
            "s00:    OUT %[portout], %[upreg]                \n" // Start of bit, bit value is in carry flag
            "        BRCS skip03                             \n" // only lower the line if the bit...
            "        OUT %[portout], %[downreg]              \n" // ...in the carry flag was zero.
            "skip03: SUBI %[bits], 1                         \n" // Decrease bit count...
            "        BRNE cont06                             \n" // ...and loop if not zero
            "        LSL __tmp_reg__                         \n" // Load the last bit into the carry flag
            "        BRCC Lx008                              \n" // Jump if last bit is zero
            "        LDI %[bits], 7                          \n" // Reset bit counter to 7
            "        OUT %[portout], %[downreg]              \n" // Force line down, even if it already was down
            "        NOP                                     \n"
            "        OUT %[portout], %[upreg]                \n" // Start of last bit of byte, which is 1
            "        SBIW %[bytes], 1                        \n" // Decrease byte count
            "        LD __tmp_reg__, %a[dataptr]+            \n" // Load next byte
            "        BRNE cont07                             \n" // Loop if byte count is not zero
            "        RJMP brk18                              \n" // Byte count is zero, jump to the end
            "Lx008:  OUT %[portout], %[downreg]              \n" // Last bit is zero
            "        LDI %[bits], 7                          \n" // Reset bit counter to 7
            "        OUT %[portout], %[upreg]                \n" // Start of last bit of byte, which is 0
            "        NOP                                     \n"
            "        OUT %[portout], %[downreg]              \n" // We know we're transmitting a 0
            "        SBIW %[bytes], 1                        \n" // Decrease byte count
            "        LD __tmp_reg__, %a[dataptr]+            \n"
            "        BRNE cont09                             \n" // Loop if byte count is not zero
            "brk18:  OUT %[portout], %[downreg]              \n"
            "                                                \n" // used to be a NOP here, but returning from the function takes long enough
            "                                                \n" // We're done.
: /* no output */
: /* inputs */
[dataptr] "e" (values),     // pointer to grb values
[upreg]   "r" (high_val),   // register that contains the "up" value for the output port (constant)
[downreg] "r" (low_val),    // register that contains the "down" value for the output port (constant)
[bytes]   "w" (size),       // number of bytes to send
[bits]    "d" (bitcount),       // number of bits/2
[portout] "I" (_SFR_IO_ADDR(WS2811_PORT)) // The port to use
    );

    sei();
}




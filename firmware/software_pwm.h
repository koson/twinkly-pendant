#if !defined SOFTWARE_PWM_H
#define      SOFTWARE_PWM_H

#include <msp430.h>
#include <stdint.h>

// LED brightness corresponds to PWM duty cycle * 256
// SMCLK @ 96 kHz cycles through count of 256 at a rate of 375/s, fast enough for LED PWM

/* By limiting the maximum dity cycle to 50% we can duplex the LEDs
 * on each resistor so that only one is on at any time.
 *
 * In each half cycle there can be up to 4 LEDs on. All LEDs that
 * are on commence illumination at the beginning of the half cycle.
 * If all 4 have different duty cycles then the maximum of 4 updates
 * to P1OUT are required to turn the LEDs off at the correct time.
 * For reasons of speed the sequence of timings and P1OUT values are
 * precoded so that the interrupt routine only needs to update a
 * single counter and update the P1OUT value and interrupt CTC
 * from the relevant arrays.
 *
 * Probably it is better to turn the LEDs on sequentially if the 
 * total on time of all LEDs is less than one half cycle.
 */

volatile uint8_t  next_LED_out[8];
volatile uint16_t next_CTC[8];

/* NB Max voltage increase over VSS that we can tolerate 
 *   = battery voltage - forward voltage of LED
 *   = 3.7 - 3.4 = 0.3 V
 *
 * According to the MSP430G2231 datasheet slas862, we can expect
 * 0.3 V increase over VSS when sinking 6 mA.
 * 
 * Therefore the minimum resistor value should be
 *   = 0.3 / 0.006 = 50 Ohms
 *
 * Next common value higher than this = 56 Ohms
 */

#endif

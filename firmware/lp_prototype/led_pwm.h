/* code to linearize percieved brightness of LEDs using PWM
 *
 * adapted from
 * http://ledshield.wordpress.com/home/
 *
 * Change brightness of LED linearly to Human eye
 * 32 step brightness using 8 bit PWM
 * brightness step 24 should be twice bright than step 12 to your eye.
 *
 * Correction calculation of luminance and brightness as described in
 * CIE 1931 report then used for CIELAB color space.
 * Where L* is lightness, Y/Yn is Luminance ratio.
 *
 * L* = 116(Y/Yn)^1/3 - 16 , Y/Yn > 0.008856
 * L* = 903.3(Y/Yn), Y/Yn <= 0.008856
*/

/* Look up table of 8 bit PWM values corresponding
 * to 32 equally spaced gradations of perceived brightness
 *
 * step  8 (25% maximum brightness) corresponds to ~3.5% duty cycle 
 * step 16 (50% maximum brightness) corresponds to ~ 17% duty cycle 
 * step 24 (75% maximum brightness) corresponds to ~ 47% duty cycle 
 *
 * The table is truncated at 24 step because there is a heavy power
 * penalty for higher duty cycles.
 *
 * With 2 LEDs sharing a single resistor there is another advantage
 * to not exceeding 50% duty cycle. Having both LEDs on at the same
 * time doubles the current and hence the voltage drop across the
 * resistor.
 */

static const uint_8t CIEL8[] = {
	0, 1, 2, 3,
	4, 5, 7, 9,
	12, 15, 18, 22,
	27, 32, 38, 44,
	51, 58, 67, 76,
	86, 96, 108, 120    /* ,
	134, 148, 163, 180,
	197, 216, 235, 255  */
};

// LED brightness corresponds to PWM duty cycle * 256
// SMCLK @ 96 kHz cycles through count of 256 at a rate of 375/s, fast enough for LED PWM
volatile uint8_t LED_duty_cycle[8];

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

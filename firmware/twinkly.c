#include "twinkly.h" 
#include "software_pwm.h"
#include "twinkly_patterns.h"

#define CYCLES_PER_PWM_TICK	6

// Watchdog timer ticks at 1500Hz
extern volatile uint16_t tick;

// LED P1OUT values
volatile uint8_t LED_out[10];

// LED CTC timings
volatile uint16_t LED_CTC[10];

// number of steps in LED timing cycle
volatile uint8_t LED_n;

// LED PWM values
uint8_t LED_pwm[8];

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

const uint8_t CIEL8[] = {
	0, 1, 2, 3,
	4, 5, 7, 9,
	12, 15, 18, 22,
	27, 32, 38, 44,
	51, 58, 67, 76,
	86, 96, 108, 120    /* ,
	134, 148, 163, 180,
	197, 216, 235, 255  */
};

static const uint8_t LED_halfcycle_end = 128;

extern volatile uint8_t update_cycle_flag;
extern volatile uint8_t pattern;
extern volatile uint8_t pattern_speed;
extern volatile uint8_t pattern_intensity;
extern volatile uint16_t lfsr;

// function declarations
static void initPins(void);
static void initClock(void);
static uint8_t LED_update_halfcycle(uint8_t, uint8_t);
static void LED_update_cycle(void);

int main(void) {
	// watchdog timer = SMCLK/64 (62.5 kHz for 4 MHz clock)
	WDTCTL = WDT_MDLY_0_064;

	// disable ADC
	ADC10CTL0 &= ~ENC;

	// initialization routines
	initPins();
	initClock();

	// enable interrupts on watchdog timer
	IE1 |= WDTIE;

	// set LEDs off
	uint8_t i;
	for (i = 0; i < 8; i++ ) {
		LED_pwm[i] = 0;
	}
	LED_update_cycle();

	// enable CTC interrupts on Timer_A
	// output OUT bit value
	TACCTL0 = OUTMOD_0 | CCIE;	

	// set up interrupt on vibration sensor
	P2IES |= VIB;	// Vibration sensor pin enable interrupt HIGH->LOW
	P2IFG &= ~VIB;	// Vibration sensor pin clear interrupt flag
	P2IE |= VIB;	// Vibration sensor pin enable interrupt

	// while waiting for VIB pin to go low
	// deep sleep
	// enable interrupts
	__bis_SR_register(LPM4_bits + GIE);

	uint8_t pos = 0;
	pattern = SPIN_FORWARD;
	//pattern_speed = LENTO;
	pattern_speed = ANDANTE;
	pattern_intensity = 4;

	for (;;) {
	
		// disable CTC interrupts
		TACCTL0 &= ~CCIE;

		if (update_cycle_flag) {
			// turn all LEDs off
			for (i = 0; i < 8; i++) {
				LED_pwm[i] = 0;
			}

	                // Turn off vibration sensor pullup to disable interrupt
			P2DIR &= ~VIB_PULLUP;
			P2REN &= ~VIB_PULLUP;

			// move through pattern steps
			uint8_t brightness, inc, j;
			switch (pattern) {
			// single LED fade sequence
			case SPIN_FORWARD:
			case SPIN_REVERSE:
				brightness = pattern_intensity * 4;
				inc = (pattern == SPIN_FORWARD) ? +1 : -1;
				for (i = 0, j = 0; i < 4; i++, j += inc) {
					LED_pwm[(pos + j) & 7] = CIEL8[brightness];
					brightness -= pattern_intensity;
				}
				pos -= inc;
				//pos &= 7u;
				break;

			// double LED fade sequence
			case DOUBLE_SPIN_FORWARD:
			case DOUBLE_SPIN_REVERSE:
				brightness = pattern_intensity * 3;
				inc = (pattern == DOUBLE_SPIN_FORWARD) ? +1 : -1;
				for (i = 0, j = 0; i < 3; i++, j += inc) {
					LED_pwm[(pos + j) & 7u] = CIEL8[brightness];
					LED_pwm[(pos + 4 + j) & 7u] = CIEL8[brightness];
					brightness -= pattern_intensity;
				}
				pos -= inc;
				//pos &= 7u;
				break;

			case FLASH:
				break;

			case GLITTER:
        			// 16 bit linear feedback shift register with taps 16 14 13 11
        			lfsr = (lfsr >> 1) ^ (-(lfsr & 1u) & 0xB400u);
				LED_pwm[lfsr & 7u] = (((lfsr >> 3) & 15u) == 0) ? CIEL8[((lfsr >> 8) & 15u)] : 0;
				break;

			default:
				;
			}

			// enable vibration sensor interrupt
			P2DIR |= VIB_PULLUP;	
			P2OUT |= VIB_PULLUP;	

			// update LED PWM cycle
			// the time it takes to do this is appreciable
			LED_update_cycle();

			// clear flag
			update_cycle_flag = 0;
		}

		// clear timer counter
		TACTL = TACLR;

		// restart timer counter
		TACCR0 = LED_CTC[0];

		// SMCLK/1, up
		TACTL = TASSEL_2 + ID_0 + MC_1;

		// enable CTC interrupts
		TACCTL0 |= CCIE;	

		// light sleep with timers enabled
		LPM0;
	}

	return 0;
}

// initialize pins
void initPins() {
	// LEDs output
	// active low, all off
	P1DIR = 0xFF;
	P1OUT = 0xFF;

	// Vibration sensor pullup output high
	P2DIR = VIB_PULLUP;
	P2OUT = VIB_PULLUP;

	// Vibration sensor input, pullup off
	//P2DIR &= ~VIB;

	// No pullups/pulldowns
	P1REN = 0;
	P2REN = 0;

	// Nothing else going on
	P1SEL = 0;
	P2SEL = 0;
}

// initialize timers
void initClock() {
/*
	// DCOCLK using slowest possible frequency = 96 kHz
	DCOCTL = 0;
	DCOCTL |= (MOD0 | DCO0);
	BCSCTL1 &= ~RSEL3;
	BCSCTL1 |= RSEL0;

*/
	// set DCOCLK to frequency ~4 MHz
	if (CALBC1_1MHZ != 0xFF) {
		// Clear DCL for BCL12
		DCOCTL = 0x00;
		// Calibration info is intact, use it.
		// hack to get somewhere between 4 and 5 MHz without further calibration
		BCSCTL1 = CALBC1_1MHZ + 5; 
		DCOCTL = CALDCO_1MHZ;
	} else {
		// calibration data missing
		for (;;) {
			P1OUT = 0xFF;
		}
	}

	// SMCLK = DCOCLK/1, MCLK = DCOCLK/1
	BCSCTL2 = DIVS_0 | SELM_0 | DIVM_0;

	// enable VLOCLK to time LED pulses
	// probably better to use the watchdog timer for this
	// Set LFXT1 to the VLO @ 12kHz
	BCSCTL3 = LFXT1S_2;

	// LFXT1 low freq, XT2 off, ACLK = XTAL/1
	BCSCTL1 &= ~XTS;
	BCSCTL1 |= XT2OFF + DIVA_0;
}

/* Pseudo code to reduce the number of LEDs turned on at the same time
 * 
 * 1. Begin at start of half cycle.
 * 2. Turn on the highest duty cycle LED.
 * 3. If no LEDs remain to be lit then go to 6.
 * 3. If the remaining time left after this LED has finished is less than the 
 *    on time of the LED with the next highest duty cycle, turn on this next LED
 *    at the same time and go to 3.
 * 4. If no LEDs remain to be lit then go to 6. 
 * 5. Wait for all LEDs currently lit to finish and goto 2.
 * 6. End.
 */

// subroutine to calculate P1OUT values and CTC timings
// for one set of 4 LEDs in half of one PWM cycle
// returns number of steps in half cycle
uint8_t LED_update_halfcycle(uint8_t halfcycle, uint8_t last) {

	// unsorted array of LED indexes
	// we wish to sort this array from the largest duty cycle to smallest
	uint8_t LED_index[4];
	
	// When in cycle to turn LEDs on/off
	uint8_t LED_start[4];
	uint8_t LED_end[4];

	uint8_t inline index(uint8_t i) { return LED_index[i] * 2 + halfcycle; }
        uint8_t inline pwm(uint8_t i) { return LED_pwm[index(i)]; }
        uint8_t inline bit(uint8_t i) { return 1 << index(i); }
	
	// count LEDs that have nonzero duty cycle
	uint8_t i, n = 0;
	for (i = 0; i < 4; i++) {
		if (LED_pwm[i * 2 + halfcycle] > 0) {
			LED_index[n++] = i;
			//if (pwm(i) > LED_halfcycle_end) {
			//	LED_pwm[index(i)] = LED_halfcycle_end;
			//}
		}
	}
	
	if (n == 0) {
		// no LEDs on
		LED_out[last] = 0xFF;
		LED_CTC[last] = LED_halfcycle_end << CYCLES_PER_PWM_TICK;
		return ++last;
	}

	// sort LEDs by PWM duty cycle, highest first
	// modified bubble sort
	uint8_t j, flag;
	for (i = 0; i < n; i++) {
		flag = 0;
		for (j = 0; j < n - 1; j++ ) {
			if (pwm(j) < pwm(j + 1)) {
				uint8_t temp = LED_index[j];
				LED_index[j] = LED_index[j + 1];
				LED_index[j + 1] = temp;
				flag = 1;
			}
		} 
		if (!flag) {
			break;
		}
	}
	
	// calculate start and end times for each LED
	uint8_t start = 0;
	uint8_t end = 0;
	i = 0;
	do {
		// start LEDs highest duty cycle first
		end += pwm(i);
		LED_start[LED_index[i]] = start;
		LED_end[LED_index[i]] = end;
	        for (j = i + 1; j < n; j++) {
			if (LED_halfcycle_end >= end + pwm(j)) {
				break;
			} 
			// start LED with lower duty cycle at the same time
			LED_start[LED_index[j]] = start;
			LED_end[LED_index[j]] = start + pwm(j);
			// order LEDs with same start time in the order they are turned off
			uint8_t k;
			for (k = j; k > i; k--) {
				//if (LED_end[LED_index[k]] < LED_end[LED_index[k - 1]]) {
					uint8_t temp = LED_index[k];
					LED_index[k] = LED_index[k - 1];
					LED_index[k - 1] = temp;
				//}
			}
		}
		i = j;
		start = end;
	} while (i < n);

	// calculate LED P1OUT values and CTC timings
	uint8_t last_out = 0xFF;
	uint8_t time = 0;
	for (i = 0; i < n; i++, last++) {
		// turn on index LED
		LED_out[last] = last_out & ~bit(i);

		// turn off LEDs that are ending now
		for (j = 0; j < i; j++) {
			if (LED_end[LED_index[j]] == time) {
				LED_out[last] |= bit(j);
			}
		}

		// turn on any other LEDs that are starting now
		for (j = i + 1; j < n; j++) {
			if (LED_start[LED_index[j]] == time) {
				LED_out[last] &= ~bit(j);
				if (LED_end[LED_index[j]] == LED_end[LED_index[i]]) {
					i = j;
				}
			} else {
				break;
			}
		}

		// store P1OUT for ongoing signals
		last_out = LED_out[last];

		// calculate CTC time for LED LED_index[i]
		LED_CTC[last] = (LED_end[LED_index[i]] - time) << CYCLES_PER_PWM_TICK;
		time = LED_end[LED_index[i]];
	}

	// is there any time left in the half cycle?
	if (LED_halfcycle_end > time) {
		// all LEDs off for last part of the half cycle
		LED_out[last] = 0xFF;
		LED_CTC[last] = (LED_halfcycle_end - time) << CYCLES_PER_PWM_TICK;
		++last;
	}

	return last;
}

// subroutine to calculate P1OUT values and CTC timings
// for 2 sets of 4 LEDs in both halves of PWM cycle
void LED_update_cycle() {
	uint8_t n1 = LED_update_halfcycle(0, 0);
	LED_n = LED_update_halfcycle(1, n1);
}

// VLOCLK can be used on it's own in LPM3 to generate interrupts on a single pin.
// *Bonus* 3 different pins can be defined as TA0.0 and 3 as TA0.1 allowing multiplexed signals.
// Watchdog timer in interval timer mode sourced from ACLK wakes to cycle through LED display sequence
// This way we can stay in LPM3 throughout the sequence
// LPM4 between sequencees of course
// http://geekswithblogs.net/codeWithoutFear/archive/2012/01/15/msp430-pwm.aspx
//
// a TA0.0 <---> TA0.1 b
//          \ / 
//           X
//          / \
// c TA0.0 <---> TA0.1 d
//
// resistors along edges (4)
//
// 4 pins can control 8 LEDs
// LEDs on opposite directions of the same arrow are adjacent (a <-> b)
// LEDs in same direction but on the other pin of the same type are opposite (a -> b/d, b -> a/c, etc)
//
// oh, a problem.
// UV leds require > 3.3V
// forget it
//
// but wait!
// we could use the 2x TA0.0 and 3x TA01.1 to control 5x LED instead of 8. It's worth thinking about.
// the overhead of software PWM seems heavy.
//
// maybe think about switching to a different chip that is calibrated to higher speeds
// and/or has more timers available.

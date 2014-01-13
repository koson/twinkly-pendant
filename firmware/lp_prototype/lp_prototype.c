#include <msp430.h>
#include "lp_prototype.h" 
#include "led_pwm.h"

// Watchdog timer ticks at 1500Hz
volatile uint16_t tick;

// LED P1OUT values
uint8_t LED_out[10];

// LED CTC timings
uint16_t LED_CTC[10];

// number of steps in LED timing cycle
uint8_t LED_n;

static const uint16_t LED_halfcycle_end;

void initPins(void);
void initClock(void);

int main(void) {
	// watchdog timer = SMCLK/64 (1500 Hz)
	WDTCTL = WDT_MDLY_0_064;

	// disable ADC
	ADC10CTL0 &= ~ENC;

	// initialization routines
	initPins();
	initClock();

	// enable interrupts on watchdog timer
	IE1 |= WDTIE;

	// set up interrupt on vibration sensor
	P2IES |= VIB;	// Vibration sensor pin enable interrupt HIGH->LOW
	P2IE |= VIB;	// Vibration sensor pin enable interrupt
	P2IFG &= ~VIB;	// Vibration sensor pin clear interrupt flag

	// while waiting for VIB pin to go low
	// deep sleep
	// enable interrupts
	__bis_SR_register(LPM4_bits + GIE);

	for (;;) {
		// flash LEDs
		// enable the interrupts that flash LEDs
		P1OUT = 0xAA;

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
	// DCOCLK using slowest possible frequency = 96 kHz
	DCOCTL = 0;
	DCOCTL |= (MOD0 | DCO0);
	BCSCTL1 &= ~RSEL3;
	BCSCTL1 |= RSEL0;

/*
	// set DCOCLK to calibrated frequency 1 MHz
	if (CALBC1_1MHZ != 0xFF) {
		// Clear DCL for BCL12
		DCOCTL = 0x00;
		// Info is intact, use it.
		BCSCTL1 = CALBC1_1MHZ;
		DCOCTL = CALDCO_1MHZ;
	} else {
		// calibration data missing
		for (;;) {
			P1OUT = 0xFF;
		}
	}
*/

	// SMCLK = DCOCLK/1, MCLK = DCOCLK/1
	BCSCTL2 = SELS0 | DIVS0 | SELM0 | DIVM0;

	// enable VLOCLK to time LED pulses
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
uint8_t LED_update_halfcycle(uint8_t* out, uint16_t* ctc, uint8_t halfcycle) {
	// unsorted array of LED indexes
	// we wish to sort this array from the largest duty cycle to smallest
	uint8_t LED_index[4];
	
	// When in cycle to turn LEDs on/off
	uint16_t LED_start[8];
	uint16_t LED_end[8];
	
	// count LEDs that have nonzero duty cycle
	uint8_t i, n = 0;
	for (i = 0; i < 4; i++) {
		LED_index[i] = i * 2 + halfcycle;
		if (LED_pwm[LED_index[i]] > 0) {
			++n;
		}
	}
	
	if (n == 0) {
		// no LEDs on
		out[0] = 0xFF;
		ctc[0] = LED_halfcycle_end;
		return 1;
	}

	// calculate start and end times for each LED
	// modified bubble sort
	uint8_t j, flag;
	for (i = 0; i < n; i++) {
		flag = 0;
		for (j = 0; j < n - 1; j++ ) {
			if (LED_pwm[LED_index[j]] < LED_pwm[LED_index[j + 1]]) {
				uint8_t temp = LED_index[j];
				LED_index[j] = LED_index[j + 1];
				LED_index[j + 1] = temp;
				flag = 1;
			}
		} 
		if (flag) {
			break;
		}
	}
	
	// calculate start and end times for each LED
	uint16_t start = 0;
	uint16_t end = 0;
	i = 0;
	do {
		// start LEDs highest duty cycle first
		end += LED_pwm[LED_index[i]];
		LED_start[LED_index[i]] = start;
		LED_end[LED_index[i]] = end;
	        for (j = i + 1; j < n; j++) {
			if ((LED_halfcycle_end - end) >= LED_pwm[LED_index[j]]) {
				break;
			} 
			// start LED with lower duty cycle at the same time
			LED_start[LED_index[j]] = start;
			LED_end[LED_index[j]] = end;
			// order LEDs in the order they are turned off
			uint8_t k;
			for (k = j; k > i; k--) {
				uint8_t temp = LED_index[k];
				LED_index[k] = LED_index[k - 1];
				LED_index[k - 1] = temp;
			}
		}
		i = j;
		start = end;
	} while (i < n);
		
	// calculate LED P1OUT values and CTC timings
	uint8_t k = 0;
	uint8_t last_out = 0xFF;
	uint16_t last_end = 0;
	for (i = 0; i < n; i++, k++) {
		out[k] = last_out & ~LED_bit[LED_index[i]];
		for (j = 0; j < i; j++) {
			if (LED_end[LED_index[j]] == LED_start[LED_index[i]]) {
				out[k] |= LED_bit[LED_index[j]];
			}
		}
		last_out = out[k];
		ctc[k] = LED_end[LED_index[i]] - last_end;
		last_end = LED_end[LED_index[i]];
		for (j = i + 1; j < n; j++) {
			if (LED_start[LED_index[j]] == LED_start[LED_index[i]]) {
				out[k] &= ~LED_bit[LED_index[j]];
				if (LED_end[LED_index[j]] == LED_end[LED_index[i]]) {
					i++;
				}
			}
		}
	}
	if (LED_halfcycle_end > last_end) {
		out[k] = 0xFF;
		ctc[k] = LED_halfcycle_end - last_end;
		++k;
	}
	return k;
}

// subroutine to calculate P1OUT values and CTC timings
// for 2 sets of 4 LEDs in both halves PWM cycle
void LED_update_cycle() {
	uint8_t n1 = LED_update_halfcycle(LED_out, LED_CTC, 0);
	uint8_t n2 = LED_update_halfcycle(&LED_out[n1], &LED_CTC[n1], 1);
	LED_n = n1 + n2;
}

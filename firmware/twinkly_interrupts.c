#include <msp430.h>
#include <legacymsp430.h>
#include "twinkly.h"
#include "twinkly_patterns.h"

extern volatile uint8_t LED_out[10];
extern volatile uint16_t LED_CTC[10];
extern volatile uint8_t LED_n;

volatile uint16_t tick;
volatile uint16_t lfsr;
volatile uint8_t LED_CTC_ctr;
volatile uint8_t update_cycle_flag;
volatile uint8_t step;
volatile uint8_t pattern;
volatile uint8_t pattern_speed;
volatile uint8_t pattern_intensity;

// Watchdog timer ticking at 1500 Hz
interrupt(WDT_VECTOR) WDT_IH (void) {
	++tick;
	uint8_t new_step = ((tick >> 8) & pattern_speed);

	if ((pattern > 0) && (step != new_step)) {
		update_cycle_flag = 1;

		// Wake CPU
                __bic_SR_register_on_exit(LPM0_bits);
	}
	step = new_step;
} 

// Timer CTC
// this interrupt needs to happen in 20 us
// in order to get 0-255 PWM at 200 Hz
// call it 14 us if it takes 6 us for the MCU to wake up 
// currently compiles to around 64 cycles
// which requires ~4.6 MHz clock, 4 MHz gives 177 Hz
// which is probably just about OK to avoid LED flicker
// could speed it up by fixing LED_n at 8 and padding LED_out
// this saves 7 cycles
// to run at 1 MHz would need 14 cycles
// not going to happen, would need hardware PWM
interrupt(TIMERA0_VECTOR) Timer0_A_IH (void) {
	// increment counter
	++LED_CTC_ctr;
	if (LED_CTC_ctr >= LED_n) {
		LED_CTC_ctr = 0;
	}
	//LED_CTC_ctr &= 0x07;
	
	// update PORT1 LEDs
	P1OUT = LED_out[LED_CTC_ctr];
	
	// clear timer counter
	TACTL = TACLR;

	// restart timer counter
	TACCR0 = LED_CTC[LED_CTC_ctr];

	// SMCLK/1, up
	TACTL = TASSEL_2 + ID_0 + MC_1;
}

// PORT2 interrupt handler
interrupt(PORT2_VECTOR) Port_2_IH (void) {
	// Don't need to test whether the interrupt is on pin VIB
	// VIB is the only input pin on PORT2
        /* if (P2IFG & VIB) { */
		// Clear interrupt flag for VIB pin
		P2IFG &= ~VIB;
		
		// Turn off vibration sensor pullup to conserve energy
		// Remember to turn it back on after debounce

		//P2REN &= ~VIB_PULLUP;

		// increment pattern counter
		++pattern;

		// set random number seed based on tick counter
		// set most significant bit to avoid 0
		lfsr = tick | 0x8000;

		// Wake CPU
                __bic_SR_register_on_exit(LPM4_bits);
        /* } */
}

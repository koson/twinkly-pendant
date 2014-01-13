#include <msp430.h>
#include <legacymsp430.h>
#include "lp_prototype.h"

extern volatile uint8_t tick;
extern uint8_t* LED_out;
extern uint16_t* LED_CTC;
extern uint8_t LED_n;

// Watchdog timer ticking at 1500 Hz
interrupt(WDT_VECTOR) WDT_IH (void) {
	tick++;
} 

// Timer CTC
interrupt


// PORT2 interrupt handler
interrupt(PORT2_VECTOR) Port_2_IH (void) {
	// Don't need to test whether the interrupt is on pin VIB
	// VIB is the only input pin on PORT2
        /* if (P2IFG & VIB) { */
		// Clear interrupt flag for VIB pin
		P2IFG &= ~VIB;
		
		// Turn off vibration sensor pullup to conserve energy
		P2DIR &= ~VIB_PULLUP;
		P2REN &= ~VIB_PULLUP;

		// Wake CPU
                __bic_SR_register_on_exit(LPM4_bits);
        /* } */
}

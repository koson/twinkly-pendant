/**
 ** Work cycle helpers and auxiliaries.
 **
 ** Copyright (c) 2013, Eugene Mamin
 ** All rights reserved.
 ** Redistribution and use in source and binary forms, with or without
 ** modification, are permitted provided that the following conditions are met:
 **
 ** 1. Redistributions of source code must retain the above copyright notice, this
 ** list of conditions and the following disclaimer.
 ** 2. Redistributions in binary form must reproduce the above copyright notice,
 ** this list of conditions and the following disclaimer in the documentation
 ** and/or other materials provided with the distribution.
 **
 ** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ** ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 ** WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 ** DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 ** ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 ** (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 ** LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 ** ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 ** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 ** SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 ** The views and conclusions contained in the software and documentation are those
 ** of the authors and should not be interpreted as representing official policies,
 ** either expressed or implied, of the FreeBSD Project
 **/

#include <msp430.h>
#include "msprf24.h"
#include "nRF24L01.h"
#include "nrf_userconfig.h"

#include "dht_mm_definitions.h"
#include "dht_mm_lib.h"

extern DHT dht22;

extern uint8_t low_power_mode_3;
extern uint8_t has_new_dht_data;

volatile unsigned int interrupt_cnt_int0 = 0;

/**
 ** int0
 ** Timer0 A0 interruption for first stage of DHT22 hand-shake.
 **/
//#pragma vector=TIMER0_A0_VECTOR
//__interrupt void Timer0_A0_IH (void) {
interrupt(TIMER0_A0_VECTOR) Timer0_A0_IH (void) {
	++interrupt_cnt_int0;

	if (interrupt_cnt_int0 == DHT_TIMER_CNT_STANDBY)
	{
		TA0CTL = TACLR;			// Clear timer counter

		P1DIR |= SNSR;			// DHT contact pin as OUT
		P1OUT &= ~SNSR;			// DHT contact pin LOW

		TA0CCR0 = 818;			// Restart timer counter
		TA0CTL = TASSEL_1 + ID_0 + MC_1;	// ACLK/1, up
	}

	if (interrupt_cnt_int0 == DHT_TIMER_CNT_INIT) {
		interrupt_cnt_int0 = 0;	// Reset interrupts counter

		TA0CTL = TACLR;		// Clear timer
		TA0CCTL0 &= ~CCIE;	// Prevent int0

		P1DIR &= ~SNSR;		// DHT contact pin as IN

		P1IES |= SNSR;		// DHT contact pin enable interrupts HIGH->LOW
		P1IE |= SNSR;		// DHT contact pin enable interrupts

		TA0CTL = TASSEL_2 + ID_0 + MC_2 + TAIE;	// Enable int1, SMCLK/1
		low_power_mode_3 = 0;
		__bic_SR_register_on_exit(LPM3_bits ^ LPM0_bits);	// LPM3 -> LPM0
	}
}

/**
 ** int1
 ** Timer A1 interruption for second stage of DHT22 hand-shake.
 **/
//#pragma vector=TIMER0_A1_VECTOR
//__interrupt void Timer0_A1_IH (void) {
interrupt(TIMER0_A1_VECTOR) Timer0_A1_IH (void) {
	switch (TA0IV) {
		case TA0IV_TAIFG:
			P1IE &= ~SNSR;	// DHT contact pin disable interrupts
			TA0CTL = TACLR;	// Clear timer
			has_new_dht_data = 1;
			__bic_SR_register_on_exit(LPM3_bits);	// wake CPU
			break;

		default:
			break;
	}
}








/**
 ** MSPRF24 interrupt routines
 **/

// SPI driver interrupt vector--USI
#ifdef __MSP430_HAS_USI__
//#pragma vector = USI_VECTOR
//__interrupt void USI_TXRX (void) {
interrupt(USI_VECTOR) USI_TXRX (void) {
	USICTL1 &= ~USIIFG;			// Clear interrupt
        __bic_SR_register_on_exit(LPM0_bits);	// Clear LPM0 bits from 0(SR)
}
#endif

// SPI driver interrupt vector--USCI F2xxx/G2xxx
#if defined(__MSP430_HAS_USCI__) && defined(RF24_SPI_DRIVER_USCI_PROVIDE_ISR)
//#pragma vector = USCIAB0RX_VECTOR
//__interrupt void USCI_RX(void) {
interrupt(USCIAB0RX_VECTOR) USCI_RX (void) {
	#ifdef RF24_SPI_DRIVER_USCI_A
	IE2 &= ~UCA0RXIE;
	#endif

	#ifdef RF24_SPI_DRIVER_USCI_B
	IE2 &= ~UCB0RXIE;
	#endif

	__bic_SR_register_on_exit(LPM0_bits);	// Clear LPM0 mode
}
#endif

// SPI driver interrupt vector--USCI F5xxx/6xxx
#if defined(__MSP430_HAS_USCI_A0__) && defined(RF24_SPI_DRIVER_USCI_PROVIDE_ISR) && defined(RF24_SPI_DRIVER_USCI_A)
//#pragma vector = USCI_A0_VECTOR
//__interrupt void USCI_A0(void) {
interrupt(USCI_A0_VECTOR) USCI_A0(void) {
	UCA0IE &= ~UCRXIE;
	__bic_SR_register_on_exit(LPM0_bits);
}
#endif

#if defined(__MSP430_HAS_USCI_B0__) && defined(RF24_SPI_DRIVER_USCI_PROVIDE_ISR) && defined(RF24_SPI_DRIVER_USCI_B)
//#pragma vector = USCI_B0_VECTOR
//__interrupt void USCI_B0(void) {
interrupt(USCI_B0_VECTOR) USCI_B0(void) {
	UCB0IE &= ~UCRXIE;
	__bic_SR_register_on_exit(LPM0_bits);
}
#endif

/**
 ** PORT2 interrupt handler
 **/
#if   nrfIRQport == 2
//#pragma vector=PORT2_VECTOR
//__interrupt void Port_2_IH (void) {
interrupt(PORT2_VECTOR) Port_2_IH (void) {
	// RF transceiver IRQ handling
        if (P2IFG & nrfIRQpin) {
		rf_irq |= RF24_IRQ_FLAGGED;
		P2IFG &= ~nrfIRQpin;			// Clear interrupt flag
                __bic_SR_register_on_exit(LPM3_bits);	// Wake CPU
        }
}
#endif

//#pragma vector = PORT1_VECTOR
//__interrupt void P1_IRQ (void) {
interrupt(PORT1_VECTOR) P1_IRQ (void) {
	// DHT22 sensor data pin interruption for values reading.
	if (P1IFG & SNSR) {
		DHT_handle_timer(&dht22, TA0R);		// Analyze current data bit

		P1IFG &= ~SNSR;				// Clear interrupt flag

		TA0CTL = TACLR;				// Clear timer
		TA0CTL = TASSEL_2 + ID_0 + MC_2 + TAIE;	// Restart timer (SMCLK/1, continuous)
	}

#if   nrfIRQport == 1
        if (P1IFG & nrfIRQpin) {
		rf_irq |= RF24_IRQ_FLAGGED;
		P1IFG &= ~nrfIRQpin;

		__bic_SR_register_on_exit(LPM3_bits);	// Wake CPU
        }
#endif
}

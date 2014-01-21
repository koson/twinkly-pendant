#if !defined TWINKLY_H
#define      TWINKLY_H

#include <msp430.h>
#include <stdint.h>

// Port1 LEDs
#define LED1	BIT0
#define LED2	BIT1
#define LED3	BIT2
#define LED4	BIT3
#define LED5	BIT4
#define LED6	BIT5
#define LED7	BIT6
#define LED8	BIT7
#define BUZZER	BIT2

// Port2 sensor
#define VIB	BIT6
#define VIB_PULLUP	BIT7

// Clock frequency
#define F_DCOCLK 4000000

#endif


lp_prototype-msp430g2231.elf:     file format elf32-msp430


Disassembly of section .text:

0000f800 <__init_stack>:
    f800:	31 40 80 02 	mov	#640,	r1	;#0x0280

0000f804 <__low_level_init>:
    f804:	15 42 20 01 	mov	&0x0120,r5	
    f808:	75 f3       	and.b	#-1,	r5	;r3 As==11
    f80a:	35 d0 08 5a 	bis	#23048,	r5	;#0x5a08

0000f80e <__do_copy_data>:
    f80e:	3f 40 00 00 	mov	#0,	r15	;#0x0000
    f812:	0f 93       	tst	r15		
    f814:	07 24       	jz	$+16     	;abs 0xf824
    f816:	82 45 20 01 	mov	r5,	&0x0120	
    f81a:	2f 83       	decd	r15		
    f81c:	9f 4f f4 f8 	mov	-1804(r15),512(r15);0xf8f4(r15), 0x0200(r15)
    f820:	00 02 
    f822:	f9 23       	jnz	$-12     	;abs 0xf816

0000f824 <__do_clear_bss>:
    f824:	3f 40 00 00 	mov	#0,	r15	;#0x0000
    f828:	0f 93       	tst	r15		
    f82a:	06 24       	jz	$+14     	;abs 0xf838
    f82c:	82 45 20 01 	mov	r5,	&0x0120	
    f830:	1f 83       	dec	r15		
    f832:	cf 43 00 02 	mov.b	#0,	512(r15);r3 As==00, 0x0200(r15)
    f836:	fa 23       	jnz	$-10     	;abs 0xf82c

0000f838 <main>:
void initPins(void);
void initClock(void);

int main(void) {
	// turn off watchdog timer
	WDTCTL = WDTHOLD | WDTPW;
    f838:	b2 40 80 5a 	mov	#23168,	&0x0120	;#0x5a80
    f83c:	20 01 

	// disable ADC
	ADC10CTL0 &= ~ENC;
    f83e:	b2 f0 fd ff 	and	#-3,	&0x01b0	;#0xfffd
    f842:	b0 01 

	// initialization routines
	initPins();
    f844:	b0 12 78 f8 	call	#0xf878	
	initClock();
    f848:	b0 12 9e f8 	call	#0xf89e	

	// set up interrupt on vibration sensor
	P2IES |= VIB;	// Vibration sensor pin enable interrupt HIGH->LOW
    f84c:	f2 d0 40 00 	bis.b	#64,	&0x002c	;#0x0040
    f850:	2c 00 
	P2IE |= VIB;	// Vibration sensor pin enable interrupt
    f852:	f2 d0 40 00 	bis.b	#64,	&0x002d	;#0x0040
    f856:	2d 00 
	P2IFG &= ~VIB;	// Vibration sensor pin clear interrupt flag
    f858:	f2 f0 bf ff 	and.b	#-65,	&0x002b	;#0xffbf
    f85c:	2b 00 

	// while waiting for VIB pin to go low
	// deep sleep
	// enable interrupts
	__bis_SR_register(LPM4_bits + GIE);
    f85e:	32 d0 f8 00 	bis	#248,	r2	;#0x00f8

	for (;;) {
		// flash LEDs
		// enable the interrupts that flash LEDs
		P1OUT = 0xAA;
    f862:	f2 40 aa ff 	mov.b	#-86,	&0x0021	;#0xffaa
    f866:	21 00 

		// sleep with only VLOCLK enabled
		LPM3;
    f868:	32 d0 d0 00 	bis	#208,	r2	;#0x00d0
    f86c:	fa 3f       	jmp	$-10     	;abs 0xf862

0000f86e <__stop_progExec__>:
    f86e:	32 d0 f0 00 	bis	#240,	r2	;#0x00f0
    f872:	fd 3f       	jmp	$-4      	;abs 0xf86e

0000f874 <__ctors_end>:
    f874:	30 40 f2 f8 	br	#0xf8f2	

0000f878 <initPins>:

// initialize pins
void initPins() {
	// LEDs output
	// active low, all off
	P1DIR = 0xFF;
    f878:	f2 43 22 00 	mov.b	#-1,	&0x0022	;r3 As==11
	P1OUT = 0xFF;
    f87c:	f2 43 21 00 	mov.b	#-1,	&0x0021	;r3 As==11

	// Vibration sensor pullup output high
	P2DIR = VIB_PULLUP;
    f880:	f2 40 80 ff 	mov.b	#-128,	&0x002a	;#0xff80
    f884:	2a 00 
	P2OUT = VIB_PULLUP;
    f886:	f2 40 80 ff 	mov.b	#-128,	&0x0029	;#0xff80
    f88a:	29 00 

	// Vibration sensor input, pullup off
	//P2DIR &= ~VIB;

	// No pullups/pulldowns
	P1REN = 0;
    f88c:	c2 43 27 00 	mov.b	#0,	&0x0027	;r3 As==00
	P2REN = 0;
    f890:	c2 43 2f 00 	mov.b	#0,	&0x002f	;r3 As==00

	// Nothing else going on
	P1SEL = 0;
    f894:	c2 43 26 00 	mov.b	#0,	&0x0026	;r3 As==00
	P2SEL = 0;
    f898:	c2 43 2e 00 	mov.b	#0,	&0x002e	;r3 As==00
}
    f89c:	30 41       	ret			

0000f89e <initClock>:

// initialize timers
void initClock() {
	// DCOCLK using slowest possible frequency = 96 kHz
	DCOCTL = 0;
    f89e:	c2 43 56 00 	mov.b	#0,	&0x0056	;r3 As==00
	DCOCTL |= (MOD0 | DCO0);
    f8a2:	f2 d0 21 00 	bis.b	#33,	&0x0056	;#0x0021
    f8a6:	56 00 
	BCSCTL1 &= ~RSEL3;
    f8a8:	f2 f0 f7 ff 	and.b	#-9,	&0x0057	;#0xfff7
    f8ac:	57 00 
	BCSCTL1 |= RSEL0;
    f8ae:	d2 d3 57 00 	bis.b	#1,	&0x0057	;r3 As==01
	}
*/

	// enable VLOCLK to time LED pulses
	// Set LFXT1 to the VLO @ 12kHz
	BCSCTL3 = LFXT1S_2;
    f8b2:	f2 40 20 00 	mov.b	#32,	&0x0053	;#0x0020
    f8b6:	53 00 

	// LFXT1 low freq, XT2 off, ACLK = XTAL/1
	BCSCTL1 &= ~XTS;
    f8b8:	f2 f0 bf ff 	and.b	#-65,	&0x0057	;#0xffbf
    f8bc:	57 00 
	BCSCTL1 |= XT2OFF + DIVA_0;
    f8be:	f2 d0 80 ff 	bis.b	#-128,	&0x0057	;#0xff80
    f8c2:	57 00 
}
    f8c4:	30 41       	ret			

0000f8c6 <Port_2_IH>:
#include "lp_prototype.h"

/**
 ** PORT2 interrupt handler
 **/
interrupt(PORT2_VECTOR) Port_2_IH (void) {
    f8c6:	0f 12       	push	r15		
P1OUT=0;
    f8c8:	c2 43 21 00 	mov.b	#0,	&0x0021	;r3 As==00
        if (P2IFG & VIB) {
    f8cc:	5f 42 2b 00 	mov.b	&0x002b,r15	
    f8d0:	3f f0 40 00 	and	#64,	r15	;#0x0040
    f8d4:	0c 24       	jz	$+26     	;abs 0xf8ee
		// Clear interrupt flag for VIB pin
		P2IFG &= ~VIB;
    f8d6:	f2 f0 bf ff 	and.b	#-65,	&0x002b	;#0xffbf
    f8da:	2b 00 
		
		// Turn off vibration sensor pullup to conserve energy
		P2DIR &= ~VIB_PULLUP;
    f8dc:	f2 f0 7f 00 	and.b	#127,	&0x002a	;#0x007f
    f8e0:	2a 00 
		P2REN &= ~VIB_PULLUP;
    f8e2:	f2 f0 7f 00 	and.b	#127,	&0x002f	;#0x007f
    f8e6:	2f 00 

		// Wake CPU
                __bic_SR_register_on_exit(LPM4_bits);
    f8e8:	b1 c0 f0 00 	bic	#240,	2(r1)	;#0x00f0, 0x0002(r1)
    f8ec:	02 00 
        }
}
    f8ee:	3f 41       	pop	r15		
    f8f0:	00 13       	reti			

0000f8f2 <_unexpected_>:
    f8f2:	00 13       	reti			

Disassembly of section .vectors:

0000ffe0 <__ivtbl_16>:
    ffe0:	74 f8 74 f8 74 f8 c6 f8 74 f8 74 f8 74 f8 74 f8     t.t.t...t.t.t.t.
    fff0:	74 f8 74 f8 74 f8 74 f8 74 f8 74 f8 74 f8 00 f8     t.t.t.t.t.t.t...

#ifndef RNA_CONFIG_H
#define RNA_CONFIG_H
/* Copyright: (c) 2013 by Curt Hartung avr@northarc.com
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

// define this in if you intend to run the RNA bus by calling rnaPoll()
// instead of interrupt driven
//#define RNA_POLL_DRIVEN

// port and number the RNA communicates over. the protocol will take
// this port over and set its direciton appropriately upon Init
#define RNA_PORT_LETTTER	B
#define RNA_PIN_NUMBER		2

// the address of this node, range is 0x1 - 0xF
#define RNA_MY_ADDRESS		0x2 // RNADeviceOLED

// for debug, set this up as an annuciator
#define rnaOn() //  (PORTD |= 0b01000000)
#define rnaOff() // (PORTD &= 0b10111111)

// define the macros below to the appropriate hardware registers for
// the target platform
#define rnaEnableINT() (GIMSK |= (1 << INT0))
#define rnaClearINT() (GIFR |= (1<<INTF0))
#define rnaDisableINT() (GIMSK &= ~(1<<INT0))
#define rnaINTArm() (MCUCR |= (1<<ISC01)); (MCUCR &= ~(1<<ISC00))
#define RNA_ISR INT0_vect

#endif


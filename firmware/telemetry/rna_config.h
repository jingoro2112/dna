#ifndef RNA_CONFIG_H
#define RNA_CONFIG_H
/*------------------------------------------------------------------------------*
 * Copyright: (c) 2013 by Curt Hartung avr@northarc.com
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

// if the RNA implementation is never going to talk back on its own
// incoming ISR, quite a bit of code can be saved in terms of dynamic
// allocation and busy-check. In other words if a packet is being
// delivered from the isr (or rnaPoll) it is an error (silently
// ignored) to attempt to queue data, with this set
#define WILL_NEVER_TALK_BACK_ON_OWN_ISR

// define this in if you intend to run the RNA bus by calling rnaPoll()
// instead of interrupt driven
//#define RNA_POLL_DRIVEN

// port and number the RNA communicates over. the protocol will take
// this port over and set its direciton appropriately upon Init
#define RNA_PORT_LETTTER	D
#define RNA_PIN_NUMBER		2

// the address of this node, range is 0x1 - 0xF
#define RNA_MY_ADDRESS		0x4

// define the macros below to the appropriate hardware registers for
// the target platform
#define rnaEnableINT() (EIMSK |= (1 << INT0))
#define rnaClearINT() (EIFR |= (1<<INTF0))
#define rnaDisableINT() (EIMSK &= ~(1<<INT0))
#define rnaINTArm() (EICRA |= (1<<ISC01)); (EICRA &= ~(1<<ISC00))
#define RNA_ISR INT0_vect

#endif

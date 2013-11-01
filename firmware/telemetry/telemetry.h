#ifndef TELEMETRY_H
#define TELEMETRY_H
/*------------------------------------------------------------------------------*
 * Copyright: (c) 2013 by Curt Hartung avr@northarc.com
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#define H_SLEEP (PORTB &= ~0b00000001)
#define H_WAKE (PORTB |= 0b00000001)
#define H2_IN2_HIGH (PORTB |= 0b00000100)
#define H2_IN2_LOW (PORTB &= ~0b00000100)
#define H2_IN1_HIGH (PORTB |= 0b00000010)
#define H2_IN1_LOW (PORTB &= ~0b00000010)
#define H1_IN2_HIGH (PORTD |= 0b10000000)
#define H1_IN2_LOW (PORTD &= ~0b10000000)
#define H1_IN1_HIGH (PORTD |= 0b01000000)
#define H1_IN1_LOW (PORTD &= ~0b01000000)


#endif

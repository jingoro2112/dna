#ifndef SRAM_H
#define SRAM_H
/*------------------------------------------------------------------------------*/
/* Copyright: (c) 2013 by Curt Hartung
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#define sramSetCSHigh() (PORTD |= 0b00100000)
#define sramSetCSLow()  (PORTD &= ~0b00100000)
#define sramSetSIHigh() (PORTC |= 0b00000001)
#define sramSetSILow()  (PORTC &= ~0b00000001)
#define sramSetSCKHigh() (PORTD |= 0b10000000)
#define sramSetSCKLow()  (PORTD &= ~0b10000000)
#define sramGetSO() (PIND & 0b01000000)
#define SRAM_WRITE_STATUS  0x01
#define SRAM_WRITE_COMMAND 0x02
#define SRAM_READ_COMMAND  0x03
#define SRAM_READ_STATUS   0x05

void sramClock( unsigned char c );
void sramClockAddress( unsigned int address );
void sramStartRead( unsigned int address );
unsigned char sramRead( unsigned int address );
void sramWrite( unsigned int address, unsigned char data );
void sramInit();
#define sramStop() sramSetCSHigh();

#endif




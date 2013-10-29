#ifndef SRAM_H
#define SRAM_H
/*------------------------------------------------------------------------------*
 * Copyright: (c) 2013 by Curt Hartung avr@northarc.com
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#include <sram_config.h>

#define SRAM_CONCAT(x, y)  x##y
#define SRAM_D_PORT(port)  SRAM_CONCAT( PORT, port )
#define SRAM_D_PIN(port)  SRAM_CONCAT( PIN, port )
#define SRAM_D_DDR(port)  SRAM_CONCAT( DDR, port )

#define SRAM_CS_PORT_L  SRAM_D_PORT( SRAM_CS_PORT )
#define SRAM_SI_PORT_L  SRAM_D_PORT( SRAM_SI_PORT )
#define SRAM_SO_PORT_L  SRAM_D_PORT( SRAM_SO_PORT )
#define SRAM_SCK_PORT_L  SRAM_D_PORT( SRAM_SCK_PORT )
#define SRAM_CS_PIN_L  SRAM_D_PIN( SRAM_CS_PORT )
#define SRAM_SI_PIN_L  SRAM_D_PIN( SRAM_SI_PORT )
#define SRAM_SO_PIN_L  SRAM_D_PIN( SRAM_SO_PORT )
#define SRAM_SCK_PIN_L  SRAM_D_PIN( SRAM_SCK_PORT )
#define SRAM_CS_DDR_L  SRAM_D_DDR( SRAM_CS_PORT )
#define SRAM_SI_DDR_L  SRAM_D_DDR( SRAM_SI_PORT )
#define SRAM_SO_DDR_L  SRAM_D_DDR( SRAM_SO_PORT )
#define SRAM_SCK_DDR_L  SRAM_D_DDR( SRAM_SCK_PORT )

#define sramSetCSHigh() (SRAM_CS_PORT_L |= (1<<SRAM_CS_PIN))
#define sramSetCSLow()  (SRAM_CS_PORT_L &= ~(1<<SRAM_CS_PIN))
#define sramSetSIHigh() (SRAM_SI_PORT_L |= (1<<SRAM_SI_PIN))
#define sramSetSILow()  (SRAM_SI_PORT_L &= ~(1<<SRAM_SI_PIN))
#define sramSetSCKHigh() (SRAM_SCK_PORT_L |= (1<<SRAM_SCK_PIN))
#define sramSetSCKLow()  (SRAM_SCK_PORT_L &= ~(1<<SRAM_SCK_PIN))
#define sramGetSO() (SRAM_SO_PIN_L & (1<<SRAM_SO_PIN))

#define SRAM_WRITE_STATUS  0x01
#define SRAM_WRITE_COMMAND 0x02
#define SRAM_READ_COMMAND  0x03
#define SRAM_READ_STATUS   0x05

#define SRAM_SIZE 0x2000 // 8k (x8)

void sramStartRead( unsigned int address );
unsigned char sramReadByte();
void sramStartWrite( unsigned int address );
void sramWriteByte( unsigned char byte );
#define sramStop() sramSetCSHigh();

void sramRead( unsigned int address, unsigned char* bytes, unsigned char len );
void sramWrite( unsigned int address, unsigned char* bytes, unsigned char len );

//unsigned char sramAtomicRead( unsigned int address );
//void sramAtomicWrite( unsigned int address, unsigned char data );

void sramInit();

#include <util/delay.h>

#endif

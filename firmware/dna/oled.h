#ifndef OLED_H
#define OLED_H
/*------------------------------------------------------------------------------*
 * Copyright: (c) 2013 by Curt Hartung avr@northarc.com
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#include <oled_config.h>

#define OLED_CONCAT(x, y)  x##y
#define OLED_D_PORT(port)  OLED_CONCAT( PORT, port )
#define OLED_D_DDR(port)  OLED_CONCAT( DDR, port )

#define OLED_CS_PORT  OLED_D_PORT( OLED_CS_PORT_LETTER )
#define OLED_CS_DDR  OLED_D_DDR( OLED_CS_PORT_LETTER )

#define OLED_RESET_PORT  OLED_D_PORT( OLED_RESET_PORT_LETTER )
#define OLED_RESET_DDR  OLED_D_DDR( OLED_RESET_PORT_LETTER )

#define oledCSHigh() (OLED_CS_PORT |= (1<<OLED_CS_PIN_NUMBER))
#define oledCSLow() (OLED_CS_PORT &= ~(1<<OLED_CS_PIN_NUMBER))
#define oledResetHigh() (OLED_RESET_PORT |= (1<<OLED_RESET_PIN_NUMBER))
#define oledResetLow() (OLED_RESET_PORT &= ~(1<<OLED_RESET_PIN_NUMBER))

void oledCommand( unsigned char cmd );
void oledClear();
#define oledPowerOff() oledCommand( 0xAE )
void oledInit( unsigned char invert );

#endif


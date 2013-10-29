#ifndef RNA_H
#define RNA_H
/*------------------------------------------------------------------------------*/
/* Copyright: (c) 2013 by Curt Hartung
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include <rna_config.h>
#include <dna_defs.h>

#define RNA_CONCAT(x, y)  x##y
#define RNA_D_PORT(port)  RNA_CONCAT( PORT, port )
#define RNA_D_PIN(port)  RNA_CONCAT( PIN, port )
#define RNA_D_DDR(port)  RNA_CONCAT( DDR, port )
#define RNA_PORT  RNA_D_PORT( RNA_PORT_LETTTER )
#define RNA_PIN  RNA_D_PIN( RNA_PORT_LETTTER )
#define RNA_DDR  RNA_D_DDR( RNA_PORT_LETTTER )

#define rnaSetHigh() (RNA_DDR &= ~(1<<RNA_PIN_NUMBER))
#define rnaSetLow() (RNA_DDR |= (1<<RNA_PIN_NUMBER))
#define rnaIsHigh() (RNA_PIN & (1<<RNA_PIN_NUMBER))
#define rnaIsLow()  (!(RNA_PIN & (1<<RNA_PIN_NUMBER)))
#define rnaSetIdle()  do { RNA_DDR &= ~(1<<RNA_PIN_NUMBER); RNA_PORT |= (1<<RNA_PIN_NUMBER); } while(0);
#define rnaSetActive()  do { RNA_DDR &= ~(1<<RNA_PIN_NUMBER); RNA_PORT &= ~(1<<RNA_PIN_NUMBER); } while(0);

void rnaInit();
unsigned char rnaShiftOutByte( unsigned char data );
unsigned char rnaShiftInByte( unsigned char high );

// utility function to do all flavors of send, use macro helper functions
unsigned char rnaSendEx( unsigned char address, unsigned char fromAddress, unsigned char *data, unsigned char len );
#define rnaSend( address, data, len ) rnaSendEx((address), RNA_MY_ADDRESS, (data), (len))
#define rnaSendSystem( address, data, len ) rnaSendEx((address), 0, (data), (len))
#define rnaProbe( address ) rnaSendEx((address), RNA_MY_ADDRESS, 0, 0)
void rnaPoll();

unsigned char rnaInputSetup( unsigned char *data, unsigned char dataLen, unsigned char from, unsigned char totalLen );
void rnaInputStream( unsigned char *data, unsigned char dataLen );

void rprint( char* string, unsigned char targetDevice );
#define rprint_oled( string ) rprint( string, RNADeviceOLED )
#define rprint_usb( string ) rprint( string, RNADeviceDNA )

void oled_clear();
void oled_pixel( char x, char y, char color );
#define oled_setPixel( x, y ) oled_pixel( x, y, 0xFF )
#define oled_resetPixel( x, y ) oled_pixel( x, y, 0x0 )
#define oled_print( buf ) rprint_oled( buf )
void oled_text( char* buf, char x, char y, char font );
void oled_line( char x0, char y0, char x1, char y1 );


#define OLED_SAFE_DELAY //_delay_ms(5);


#endif

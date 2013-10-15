#ifndef USB_H
#define USB_H
/*------------------------------------------------------------------------------*
 * Copyright: (c) 2013 by Curt Hartung avr@northarc.com
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#include <usbdrv.h>
#include <util/delay.h>

// The DNA USB interface provides standard calls and libraries used for
// communicating over USB with the provided loader library


// messageCallback will be called for each 8-byte message, and its
// contents will be returned, do not modify message[0]
#define dnaUsbInit() \
	usbInit(); \
	usbDeviceDisconnect(); \
	_delay_ms( 250 ); \
	usbDeviceConnect(); \

void dnaUsbQueueData( unsigned char* data, unsigned char len );
void dnaUsbQueueHandle( unsigned char handle, unsigned char len );

extern volatile unsigned char g_sendQueueLen;
#define dnaUsbSendQueueBusy() (g_sendQueueLen != 0)

unsigned char dnaUsbInputSetup( unsigned char totalSize, unsigned char *data, unsigned char len );
void dnaUsbInputStream( unsigned char *data, unsigned char len );
void dnaUsbCommand( unsigned char command, unsigned char data[5] );

#define MAX_OUTSTANDING_PRINT_STRING_BYTES 200
void usbprint( char* string, unsigned char len );
			  
#endif

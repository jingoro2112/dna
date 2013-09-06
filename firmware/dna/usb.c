/* Copyright: (c) 2013 by Curt Hartung
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#include "../../usbdrv/usbdrv.c" // including the source code saves space

#include "dna.h"
#include "usb.h"
#include "rna.h"

#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>

//------------------------------------------------------------------------------
const PROGMEM char usbHidReportDescriptor[USB_CFG_HID_REPORT_DESCRIPTOR_LENGTH] =
{
	0x06, 0x00, 0xff,              // USAGE_PAGE (Generic Desktop)
	0x09, 0x01,                    // USAGE (Vendor Usage 1)

	0xa1, 0x01,                    // COLLECTION (Application)
	0x15, 0x00,                    // LOGICAL_MINIMUM (0)
	0x26, 0xff, 0x00,              // LOGICAL_MAXIMUM (255)
	0x75, 0x08,                    // REPORT_SIZE (8)

	0x85, REPORT_DNA,			   // REPORT_ID
	0x95, REPORT_DNA_SIZE,         // REPORT_COUNT fits in one go
	0x09, 0x00,                    // USAGE (Undefined)
	0xb2, 0x02, 0x01,              // FEATURE (Data,Var,Abs,Buf)

	0x85, REPORT_DNA_DATA,		   // REPORT_ID
	0x95, REPORT_DNA_DATA_SIZE,    // REPORT_COUNT
	0x09, 0x00,                    // USAGE (Undefined)
	0xb2, 0x02, 0x01,              // FEATURE (Data,Var,Abs,Buf)

	0xc0                           // END_COLLECTION
};

volatile unsigned char g_sendQueueLen; // global so it can be inspected by an extern from the header

static unsigned char s_sendQueuePosition;
static unsigned char *s_sendQueueBuffer;
static unsigned char s_dataTransferRemaining;
static unsigned char s_transferType;

//------------------------------------------------------------------------------
unsigned char usbFunctionSetup( unsigned char data[8] )
{
	if( ((usbRequest_t *)data)->bRequest == USBRQ_HID_SET_REPORT )
	{
		return USB_NO_MSG;
	}

	// USBRQ_HID_GET_REPORT is the only other supported message

	s_transferType = ((usbRequest_t *)data)->wValue.bytes[0]; // what kind of message did we just get
	return 0xFF;
}

//------------------------------------------------------------------------------
unsigned char usbFunctionWrite( unsigned char *data, unsigned char len )
{
	unsigned char consumed = 0;

	if ( s_dataTransferRemaining == 0 )
	{
		// start of a new transfer, what are we doing?
		if ( *data == REPORT_DNA ) // it is a command
		{
			if ( data[1] == USBCommandUser )
			{
				dnaUsbCommand( data[2], data + 3 );
			}
			else if ( data[1] == USBCommandEnterBootloader ) // soft entry to bootloader?
			{
				wdt_enable( WDTO_15MS ); // light the fuse
			}
/*
			if ( data[1] == USBCommandRNACommand )
			{
				consumed = 3; // USB, command, address
				s_status = (data[2] << 4) | Status_DataToRNA;
			}
			
			if ( data[1] == USBCommandWriteData ) // data send?
			{
				consumed = dnaUsbInputSetup( data + 2, len - 2 ) + 2;
			}
*/
			return 1; // commands are handled in one go
		}

		// it was Report_DNA_Data: throw it to the app
		s_dataTransferRemaining = REPORT_DNA_DATA_SIZE + 1;
		consumed = dnaUsbInputSetup( data[1], data + 2, len - 2 ) + 2;
	}

	if ( consumed < len )
	{
//		if ( s_status & Status_DataToRNA )
//		{
//			rnaSend( s_status & 0xF, data + consumed, len - consumed );
//		}
//		else
		{
			dnaUsbInputStream( data + consumed, len - consumed );
		}
	}

	if ( !(s_dataTransferRemaining -= len) )
	{
		return 1;
	}
						  
	return 0;
}

//------------------------------------------------------------------------------
unsigned char usbFunctionRead( unsigned char *data, unsigned char len )
{
	int i = 0; // default to 'nothing consumed'
	
	if ( !s_dataTransferRemaining ) // if this is the first request, determine what we are doing
	{
		if ( s_transferType == REPORT_DNA_DATA )
		{
			// straight data, shift up whatever is queued (if anything)
			s_dataTransferRemaining = (REPORT_DNA_DATA_SIZE - 1); // all data requests are fixed size, set up streaming
			data[0] = REPORT_DNA_DATA; // type
			data[1] = g_sendQueueLen; // how much was waiting to be shifted
			i = 2; // 2 bytes consumed
		}
		else
		{
			// status request, report hardware/firmware version
			data[0] = REPORT_DNA;
			data[1] = DNA_AT84_v1_00;
			data[2] = g_sendQueueLen;
			return len; // requests are sized to be single-pass, no further calls will be made after this
		}
	}

	// fill in available space with any available data
	for( ; i<len ; i++ )
	{
		if ( g_sendQueueLen )
		{
			data[i] = s_sendQueueBuffer[s_sendQueuePosition++];
			if ( s_sendQueuePosition == g_sendQueueLen )
			{
				g_sendQueueLen = 0;
			}
		}
		s_dataTransferRemaining--;
	}
	
	return len;
}

//------------------------------------------------------------------------------
void dnaUsbQueueData( unsigned char* data, unsigned char len )
{
	cli();
	s_sendQueuePosition = 0;
	g_sendQueueLen = len;
	s_sendQueueBuffer = data;
	sei();
}


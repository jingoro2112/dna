/* Copyright: (c) 2013 by Curt Hartung
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#include "dna.h"
#include "usb.h"
#include "../../usbdrv/usbdrv.c"


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

	0x85, Report_Command,		   // REPORT_ID
	0x95, 0x41,                    // REPORT_COUNT (64 + 1)
	0x09, 0x00,                    // USAGE (Undefined)
	0xb2, 0x02, 0x01,              // FEATURE (Data,Var,Abs,Buf)

	0xc0                           // END_COLLECTION
};

static unsigned char s_status;

volatile unsigned char g_sendQueueLen;
volatile static unsigned char s_sendQueuePosition;
volatile static unsigned char *s_sendQueueBuffer;

static unsigned char s_dataTransferRemaining;

//------------------------------------------------------------------------------
unsigned char usbFunctionSetup( unsigned char data[8] )
{
	usbRequest_t *request = (usbRequest_t *)data;
	
	if( request->bRequest == USBRQ_HID_SET_REPORT )
	{
		s_status |= Status_CommandToMCUSetup;
		return USB_NO_MSG;
	}
	// USBRQ_HID_GET_REPORT is the only other supported message

	s_status |= Status_DataFromMCUSetup;
	return 0xFF;
}

//------------------------------------------------------------------------------
unsigned char usbFunctionWrite( unsigned char *data, unsigned char len )
{
	if ( s_status & Status_CommandToMCUSetup ) // a command is being sent down
	{
		s_dataTransferRemaining = 66;
		s_status &= ~Status_CommandToMCUSetup;

		if ( data[1] == USBCommandEnterBootloader ) // soft entry to bootloader?
		{
			wdt_enable( WDTO_15MS ); // light the fuse
		}

		if ( data[1] == USBCommandWriteData ) // data send?
		{
			unsigned char consumed = 2;
			consumed += dnaUsbInputSetup( data + 2, len - 2 );
			if ( consumed < len )
			{
				dnaUsbInputStream( data + consumed, len - consumed );
			}
		}
	}

	dnaUsbInputStream( data, len );

	if ( !(s_dataTransferRemaining -= len) )
	{
		return 1;
	}
						  
	return 0;
}

//------------------------------------------------------------------------------
unsigned char usbFunctionRead( unsigned char *data, unsigned char len )
{
	unsigned char i = 0;

	if ( s_status & Status_DataFromMCUSetup )
	{
		s_status &= ~Status_DataFromMCUSetup;
		data[i++] = Report_Command;
		data[i] = DNA_AT84_v1_00;
		if ( g_sendQueueLen )
		{
			data[i] |= DNA_RTS;
		}
		i++;
	}

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
		else
		{
			data[i] = 0; // be nice, null the data that is not written
		}
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


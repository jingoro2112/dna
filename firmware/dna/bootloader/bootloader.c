/* Copyright: (c) 2013 by Curt Hartung
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#include "../dna_defs.h"
#include "../../../usbdrv/usbdrv.c"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/boot.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <util/delay.h>

//------------------------------------------------------------------------------
// figuring out this table is deep, dark USB magic. there is an online
// tool to help you and some tutorials, but it was still a trial/error
// process.
// In order to save space, the bootloader defines only one report that
// is always 65 bytes (actually 66, add one for the report type, which
// is prepended to the data in the HID standard)
// a command packet looks like this:
// [ 8-bit type ][ 8-bit command ][ 64 bytes data .........  ]
const PROGMEM char usbHidReportDescriptor[USB_CFG_HID_REPORT_DESCRIPTOR_LENGTH] =
{
	0x06, 0x00, 0xff,		// USAGE_PAGE (Generic Desktop)
	0x09, 0x01,				// USAGE (Vendor Usage 1)
	
	0xa1, 0x01,				// COLLECTION (Application)
	0x15, 0x00,				// LOGICAL_MINIMUM (0)
	0x26, 0xff, 0x00,		// LOGICAL_MAXIMUM (255)
	0x75, 0x08,				// REPORT_SIZE (8)

	0x85, Report_Command,	// REPORT_ID
	0x95, 0x06,				// REPORT_COUNT (ID, index/command, word[2], word[2])
	0x09, 0x00,				// USAGE (Undefined)
	0xb2, 0x02, 0x01,		// FEATURE (Data,Var,Abs,Buf)

	0xc0					// END_COLLECTION
};

//static int g_pageZero[32]; // retain page zero (ISR) in RAM, commit only after load is complete and validated
//static unsigned char g_pageZeroIndex;
static unsigned char s_replyBuffer[4];

//------------------------------------------------------------------------------
void commitPage( unsigned int page )
{
	boot_page_erase( page );
	boot_spm_busy_wait();
	boot_page_write( page );
	boot_spm_busy_wait();
}

//------------------------------------------------------------------------------
unsigned char usbFunctionSetup( unsigned char data[8] )
{
	if( ((usbRequest_t *)data)->bRequest == USBRQ_HID_SET_REPORT )
	{
		return USB_NO_MSG;
	}

	// Report_Command inferred; only one command is legal, so don't
	// bother parsing it

	s_replyBuffer[0] = Report_Command;
	s_replyBuffer[1] = BOOTLOADER_DNA_AT84_v1_00;
	usbMsgPtr = (usbMsgPtr_t)s_replyBuffer;

	return 4;
}

//------------------------------------------------------------------------------
unsigned char usbFunctionWrite( unsigned char *data, unsigned char len )
{
	data++; // skip header, which in this case is always 'Report_Command'
	unsigned char index = *data++;

	cli();

	if ( index == BootloaderCommandCommitPage ) // faux index indicating we should commit the data we've been loading
	{
		unsigned int page = *(unsigned int*)data; // where to write this page
		data += 2;

		commitPage( page );
		
		if ( page == 0 )
		{
			// okay that was page zero (by definition the last uploaded
			// page, since it will render our current ISR unusable)
			// checksum the entire in-place image before executing it
			index = 0;
			for( unsigned int i=0; i<*(unsigned int*)data; i++ )
			{
				index += pgm_read_byte( i );
			}

			data += 2;
			
			if ( index != *data )
			{
				asm volatile ("ijmp" ::"z" (BOOTLOADER_ENTRY)); // WARNING! DANGER! RESTART!
			}
			else
			{
				asm volatile ("ijmp" ::"z" (0)); // in with both feet...
			}
		}
	}
	else
	{
		boot_page_fill( index, *(unsigned int *)data );
		boot_page_fill( index + 2, *(unsigned int *)(data + 2) );
	}

	sei();

	return 1; // no need to check stream, command is always exactly 7 bytes long to fit in a single pass
}

// manually locate the reset vector, since we took out the automatic
// table generation in the custom linking file
//------------------------------------------------------------------------------
void ResetVector (void) __attribute__ ((naked)) __attribute__ ((section (".reset"))); 
void ResetVector (void) 
{ 
	asm volatile ("rjmp __ctors_end");
}

//------------------------------------------------------------------------------
// the process for figuring this out is rather ugly, after compiling,
// look in the .disassembly file for the __vector_1 symbol, then
// hand-compile an indirect call to it, and install that at the INT0
// interrupt vector, remembering to divide the address by 2 to get
// PC space, not address space!
const PROGMEM int isrJump[] =
{
	0xC008, // 08 c0  rjmp +16  -- trampoline for RESET vector

	0x93EF, // ef 93  push r30   -- INT0 redirected to high ram with an ICALL
	0x93FF, // ff 93  push r31


	
//	0xEFEE, // ee ef  ldi r30, 0xFE 
//	0xE0FC, // fc e0  ldi r31, 0x0C


0xE1EE,//      ee e1           ldi     r30, 0x1E       ; 30
0xE0FD,//1cc6:       fd e0           ldi     r31, 0x0D       ; 13

	
	0x9509, // 09 95  icall         -- an icall, so it can be reached WAAAY at the top of Flash
	0x91FF, // ff 91  pop r31
	0x91EF, // ef 91  pop r30
	0x9508, // 08 95  ret

	0xE8E0, // e0 e8  ldi r30, 0x80	-- ijmp to the bootloader
	0xE0FC, // fc e0  ldi r31, 0x0C
	0x9409, // 09 94  ijmp
};

//------------------------------------------------------------------------------
int __attribute__((OS_main)) main(void)
{
	// Once the bootloader has been entered, hack up the interrupt vector table
	
	const int* isr = isrJump; // read array from program memory, saves copy step
	unsigned char i;
	for( i=0; i<24; i+=2 )
	{
		boot_page_fill( i, pgm_read_word(isr++) ); // rewrite the ISR to jump way up to our USB int0 routine
	}

	commitPage( 0 );
	
	DDRA = 0b10000000;

	usbInit();
	usbDeviceDisconnect();
	_delay_ms(250);
	usbDeviceConnect();

	sei();

	unsigned int c;
	for(;;)
	{
		c = 0;
		PORTA ^= 0b10000000;
		while( c < 13000 ) // 13000 experimentally determined to be "omg feed me!" blink rate
		{
			c++;
			usbPoll();
		}
	}
}

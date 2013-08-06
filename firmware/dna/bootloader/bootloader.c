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
	0x95, 0x41,				// REPORT_COUNT (64 + 1)
	0x09, 0x00,				// USAGE (Undefined)
	0xb2, 0x02, 0x01,		// FEATURE (Data,Var,Abs,Buf)

	0xc0					// END_COLLECTION
};

int g_addressBase; // page base currently being loaded
static int g_byteCountIn; // how much data is remaining to tranfer in the current code page
static int g_pageZero[32]; // retain page zero (ISR) in RAM, commit only after load is complete and validated
static int g_pageZeroIndex;
unsigned char g_codeChecksum; // must match before user app is entered
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
		g_byteCountIn = 0;
		return USB_NO_MSG;
	}

//	USBRQ_HID_GET_REPORT is the only other supported method

	s_replyBuffer[0] = Report_Command;
	s_replyBuffer[1] = BOOTLOADER_DNA_AT84_v1_00;

	// Report_Command inferred; only one command is legal, so don't
	// bother parsing it
		
#ifdef RELOCATE
	g_addressBase = 0; // reset loader
#else
	g_addressBase = 0x800; // reset loader
#endif

//	g_codeChecksum = 0;
	usbMsgPtr = (usbMsgPtr_t)s_replyBuffer;
	return 4;
}

//------------------------------------------------------------------------------
unsigned char usbFunctionWrite( unsigned char *data, unsigned char len )
{
	if ( !g_byteCountIn ) // a command is being sent down
	{
		if ( data[1] == USBCommandEnterApp ) // being asked to commit the code update and jump to the app
		{
			cli();

			if (  data[2] == g_codeChecksum ) // was the checksum accurate?
			{
				// now safe to install page 0 (interrupt vector table)
				// overwriting our USB hack
				for( len=0; len<32; len++ ) // re-using param saves stack-frame constructions opcodes
				{
					boot_page_fill( len*2, g_pageZero[len] );
				}
				
				commitPage( 0 );
				
				asm volatile ("ijmp" ::"z" (0)); // jump to code start
			}

			// okay here is the thing, if the checksum is not accurate,
			// there should be some cleanup to give 'another chance'
			// but there is not enough program space left to do that,
			// so I am settling for leaving interrupts disabled. This
			// will have the effect of leaving the board 'frozen',
			// requiring a power cycle. messy but 100% safe, and an
			// indication that "something has gone wrong"
		}

		// else it is a code page, the only other recognized command
		// for the bootloader
		
		data += 2;
		len -= 2;
	}
		
	for( ; len ; len -= 2, g_byteCountIn += 2 )
	{
		uint16_t w = *data++;
		w += (*data++) << 8;
		g_codeChecksum += (w >> 8) + w;

		if ( g_addressBase == 0 ) // don't commit page zero until very last, or our interrupts will be lost
		{
			g_pageZero[g_pageZeroIndex++] = w;
		}
		else
		{
			boot_page_fill( g_addressBase + g_byteCountIn, w );
		}
	}

	if ( g_byteCountIn & 0x40 ) // done with this block?
	{
		if ( g_addressBase && (g_addressBase <= 0x18C0) )
		{
			commitPage( g_addressBase );
		}
		g_addressBase += 64;
		return 1;
	}

	return 0;
}

#ifdef RELOCATE
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
	0x08C0, // c0 08  rjmp +8    -- rjmp to trampoline
	0x93EF, // ef 93  push r30   -- INT0, push r30/31
	0x93FF, // ff 93  push r31
	0xEEE7, // e7 ee  ldi r30, 0xE7 -- load them with the ISR vector for..
	0xE0FC, // fc e0  ldi r31, 0x0C
	0x9509, // 09 95  icall         -- an icall, so it can be reached WAAAY at the top of Flash
	0x91FF, // ff 91  pop r31
	0x91EF, // ef 91  pop r30
	0x9508, // 08 95  ret
	0xE6E0, // e0 e7  ldi r30, 0x60	-- ijmp to the bootloader
	0xE0FC, // fc e0  ldi r31, 0x0C
	0x9409, // 09 94  ijmp
};

#endif

//------------------------------------------------------------------------------
int __attribute__((noreturn)) main(void)
{
#ifdef RELOCATE
	
	// Once the bootloader has been entered, hack up the interrupt vector table
	
	const int* isr = isrJump; // read array from program memory, saves copy step
	unsigned char i;
	for( i=0; i<24; i+=2 )
	{
		boot_page_fill( i, pgm_read_word(isr++) ); // rewrite the ISR to jump way up to our USB int0 routine
	}

	commitPage( 0 );
	
#endif

	usbInit();
	usbDeviceDisconnect();
	_delay_ms( 250 );
	usbDeviceConnect();

	sei();

	DDRA |= 0b10000000;
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

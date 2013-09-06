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

// for some reason this compiles larger under Win-AVR than linux
// avr-gcc, not a whole lot larger but enough so it doesn't fit, so its
// linux-only to compile the image.

//------------------------------------------------------------------------------
// figuring out this table is deep, dark USB magic. there is an online
// tool to help you and some tutorials, but it was still a trial/error
// process.
const PROGMEM char usbHidReportDescriptor[USB_CFG_HID_REPORT_DESCRIPTOR_LENGTH] =
{
	0x06, 0x00, 0xff,		// USAGE_PAGE (Generic Desktop)
	0x09, 0x01,				// USAGE (Vendor Usage 1)
	
	0xa1, 0x01,				// COLLECTION (Application)
	0x15, 0x00,				// LOGICAL_MINIMUM (0)
	0x26, 0xff, 0x00,		// LOGICAL_MAXIMUM (255)
	0x75, 0x08,				// REPORT_SIZE (8)

	0x85, REPORT_DNA,		// REPORT_ID
	0x95, REPORT_DNA_SIZE,	// REPORT_COUNT (ID, index/command, word[2], word[2])
	0x09, 0x00,				// USAGE (Undefined)
	0xb2, 0x02, 0x01,		// FEATURE (Data,Var,Abs,Buf)

	0xc0					// END_COLLECTION
};

static unsigned char s_replyBuffer[4]; // some global space for reply/status buffer

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

	// Report_DNA inferred; only one command is legal, so don't
	// bother parsing it

	s_replyBuffer[0] = REPORT_DNA;
	s_replyBuffer[1] = BOOTLOADER_DNA_AT84_v1_00;
	usbMsgPtr = (usbMsgPtr_t)s_replyBuffer;

	return 4;
}

//------------------------------------------------------------------------------
unsigned char usbFunctionWrite( unsigned char *data, unsigned char len )
{
	// if interrupts are enabled, funky things happen on some USB
	// busses, theory: ISR being entered and data being changed before
	// this routing can deal with it
	cli(); 

	data++; // skip header, which in this case is always 'Report_DNA'

	unsigned char index = *data++; // gnab some context

	if ( index == BootloaderCommandCommitPage ) // faux index indicating we should commit the data we've been loading
	{
		// commit the boot_page to the page of FLASH we have been directed to
		unsigned int page = *(unsigned int*)data;
		data += 2;

		if ( page < BOOTLOADER_ENTRY*2 ) // don't overwrite the bootloader! no matter what the master thinks
		{
			commitPage( page );

			if ( page == 0 )
			{
				// okay that was page zero (by definition the last uploaded
				// page, since it will render our current ISR unusable)
				// this implies code-complete and jump, so checksum the
				// image and off we go!
				
				index = 0;
				for( unsigned int i=0; i<*(unsigned int*)data; i++ )
				{
					index += pgm_read_byte( i );
				}

				data += 2;

				if ( index != *data )
				{
					asm volatile ("ijmp" ::"z" (BOOTLOADER_ENTRY)); // WARNING! DANGER! REBOOT!
				}
				else
				{
					asm volatile ("ijmp" ::"z" (0)); // in with both feet...
				}
			}
		}
	}
	else
	{
		// was an actual offset into the boot_page table, fill in the two data words
		boot_page_fill( index, *(unsigned int *)data );
		index += 2;
		data += 2;
		boot_page_fill( index, *(unsigned int *)data );
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
	// RESET vector trampoline
	0xC008, // 08 c0  rjmp +16

	// ISR trampoline for V-USB
	0x93EF, // ef 93  push r30
	0x93FF, // ff 93  push r31
	0xE1EE, // ee e1  ldi r30, 0x1E
	0xE0FD, // fd e0  ldi r31, 0x0D
	0x9509, // 09 95  icall
	0x91FF, // ff 91  pop r31
	0x91EF, // ef 91  pop r30
	0x9508, // 08 95  ret

	// RESET trampoline to bootloader

	0xEAE0, // e0 ea  ldi r30, 0xA0       ; 160
	0xE0FC, // fc e0  ldi r31, 0x0C
	0x9409, // 09 94  ijmp
};

//------------------------------------------------------------------------------
int __attribute__((OS_main)) main(void)
{
	// Once the bootloader has been entered, hack up the interrupt
	// vector table with our trampolines
	
	const int* isr = isrJump; // read array from program memory, saves copy step
	unsigned char i;
	for( i=0; i<24; i+=2 )
	{
		boot_page_fill( i, pgm_read_word(isr++) ); // rewrite the ISR to jump way up to our USB int0 routine
	}

	commitPage( 0 );
	
	DDRA = 0b10000000; // enable LED, gotta tell the world we are waiting for code with rapid flash

	// set up USB
	usbInit();
	usbDeviceDisconnect();
	_delay_ms(250);
	usbDeviceConnect();

	sei(); // let 'er rip!

	unsigned int c;
	for(;;)
	{
		c = 0;
		PORTA ^= 0b10000000;
		while( c < 13000 ) // 13000 experimentally determined to be the "omg feed me!" blink rate
		{
			c++;
			usbPoll();
		}
	}
}

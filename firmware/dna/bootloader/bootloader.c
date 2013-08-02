#include "../dna_defs.h"
#include "../../../usbdrv/usbdrv.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/boot.h>
#include <avr/pgmspace.h>
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

static int g_addressBase = 0;
static int g_byteCountIn;
static int g_pageZero[32];
static int g_pageZeroCounter;

//------------------------------------------------------------------------------
void commitPage( unsigned int page )
{
	boot_page_erase( page );
	boot_spm_busy_wait();
	boot_page_write( page );
	boot_spm_busy_wait();
}

static unsigned char s_replyBuffer[66];

//------------------------------------------------------------------------------
unsigned char usbFunctionSetup( unsigned char data[8] )
{
	usbRequest_t *request = (usbRequest_t *)data;

	if( request->bRequest == USBRQ_HID_SET_REPORT )
	{
		g_byteCountIn = 0;
		return USB_NO_MSG;
	}
	else// if ( request->bRequest == USBRQ_HID_GET_REPORT )
	{
		// Report_Command implied only one command is legal, so don't
		// bother parsing it
		s_replyBuffer[0] = Report_Command;
		s_replyBuffer[1] = BOOTLOADER_DNA_AT84_v1_00;
		
#ifdef RELOCATE
		g_addressBase = 0; // reset loader
#else
		g_addressBase = 0x800; // reset loader
#endif
		usbMsgPtr = (usbMsgPtr_t)s_replyBuffer;
		return 4;
	}
	
	return 0;
}

//------------------------------------------------------------------------------
unsigned char usbFunctionWrite( unsigned char *data, unsigned char len )
{
	if ( !g_byteCountIn ) // a command is being sent down
	{
		if ( data[1] == USBCommandEnterApp )
		{
			cli();

			// now safe to install page 0 (interrupt vector table)
			// overwriting our USB hack
			for( len=0; len<32; len++ ) // re-using param saves stack-frame constructions opcodes
			{
				boot_page_fill( len*2, g_pageZero[len] );
			}

			commitPage( 0 );

			asm volatile ("ijmp" ::"z" (0)); // jump to code start
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

		if ( g_addressBase == 0 )
		{
			g_pageZero[g_pageZeroCounter++] = w;
		}
		else
		{
			boot_page_fill( g_addressBase + g_byteCountIn, w );
		}
	}

	if ( g_byteCountIn & 0x40 )
	{
		if ( g_addressBase )
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
	0x93EF, // ef 93  push r30
	0x93FF, // ff 93  push r31
	0xEDE8, // e8 ed  ldi r30, 0xD8
	0xE0FC, // fc e0  ldi r31, 0x0C
	0x9509, // 09 95  icall
	0x91FF, // ff 91  pop r31
	0x91EF, // ef 91  pop r30
	0x9508, // 08 95  ret
};

#endif

//------------------------------------------------------------------------------
int __attribute__((noreturn)) main(void)
{
	unsigned char i;

#ifdef RELOCATE
	// preserve reset vector in case it all goes pear-shaped.. since
	// tha DNA spec requires __init to be the very first thing called
	// in all cases, if the bootloader exits, the main app will be
	// called again. it will be screwed because we rewrote its INT0,
	// BUT since __init has to be called first, the bootloader can
	// still be entered with the programming dongle, but only if we
	// write back what was there by reading it first
	boot_page_fill( 0, pgm_read_word(0) );

	const int* isr = isrJump; // read array from program memory, saves copy step
	for( i=2; i<18; i+=2 )
	{
		boot_page_fill( i, pgm_read_word(isr++) ); // rewrite the ISR to jump way up to our USB int0 routine
	}

	commitPage( 0 );
#endif
	
	usbInit();
	usbDeviceDisconnect();
	_delay_ms(250);
	usbDeviceConnect();

	sei();

	for(;;)
	{
		usbPoll();
	}
}

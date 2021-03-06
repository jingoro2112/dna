/*------------------------------------------------------------------------------*
 * Copyright: (c) 2013 by Curt Hartung avr@northarc.com
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#include <dna.h>
#include <cfal12832.h>
#include <i2c_brute.h>
#include <24c512.h>
#include <rna.h>
#include <rna_packet.h>
#include <sram.h>
#include <galloc.h>
#include <dstring.h>

#include <util/delay.h>
#include <avr/eeprom.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#include "eeprom_image.h"
#include "eeprom_types.h"
#include "frame.h"
#include "text.h"
#include "menu.h"

#define true 1
#define false 0

#define NUMBER_OF_BIT_ENTRIES 2
struct Bits
{
	volatile uint8 b0:1;
	volatile uint8 b1:1;
	volatile uint8 b2:1;
	volatile uint8 b3:1;
	volatile uint8 b4:1;
	volatile uint8 b5:1;
	volatile uint8 b6:1;
	volatile uint8 b7:1;
} volatile bits[NUMBER_OF_BIT_ENTRIES];
#define displayDirty		(bits[0].b0)
#define displayBlit			(bits[0].b1)
#define clearDisplay		(bits[0].b2)

#define SRAM_CONSOLE_LINE_BUFFER_START 512
#define SRAM_CONSOLE_LINE_BUFFER_WIDTH 30
#define SRAM_CONSOLE_LINE_BUFFER_LINES 5
#define SRAM_CONSOLE_LINE_FONT 5
#define SRAM_CONSOLE_LINE_SPACING 6
unsigned char currentConsoleLine;

unsigned char rnaCommand;
unsigned char rnaFrom;
unsigned char rnaDataExpected;
unsigned char rnaDataReceived;
unsigned char rnaPacket;

unsigned char numberOfFonts;
unsigned int dataBlockOrigin;

#define RNA_PACKET_QUEUE_SIZE 32
#define RNA_PACKET_QUEUE_BYTE_SIZE 200
unsigned char packetQueuePos;
unsigned char packetQueue[RNA_PACKET_QUEUE_SIZE];
unsigned int loadErrors;

unsigned char frameMode;

struct OledSettings settings;

//------------------------------------------------------------------------------
void loadEEPROMConstants()
{
	for( unsigned int i=0; i<sizeof(settings); i++ )
	{
		((uint8*)&settings)[i] = eeprom_read_byte( (uint8*)i );
	}
}

//------------------------------------------------------------------------------
void saveEEPROMConstants()
{
	for( unsigned int i=0; i<sizeof(settings); i++ )
	{
		eeprom_write_byte( (uint8*)i, *(((uint8*)&settings) + i) );
		eeprom_busy_wait();
	}
	loadEEPROMConstants(); // be sure to do any translations that occur on load
}

//------------------------------------------------------------------------------
void consolePrint( char *string )
{
	sramStartWrite( SRAM_CONSOLE_LINE_BUFFER_START + (currentConsoleLine * SRAM_CONSOLE_LINE_BUFFER_WIDTH) );

	for( unsigned char c=0; string[c] && c<(SRAM_CONSOLE_LINE_BUFFER_WIDTH - 1); c++ )
	{
		sramWriteByte( string[c] );
	}
	sramWriteByte( 0 );
	sramStop();

	if ( ++currentConsoleLine >= SRAM_CONSOLE_LINE_BUFFER_LINES )
	{
		currentConsoleLine = 0;
	}
}

//------------------------------------------------------------------------------
unsigned char rnaInputSetup( unsigned char *data, unsigned char dataLen, unsigned char from, unsigned char packetLen )
{
	rnaCommand = *data;
	rnaDataExpected = packetLen - 1;
	rnaDataReceived = 0;
	rnaFrom = from;
	char *ptr;
	rnaPacket = galloc( rnaDataExpected, (void**)&ptr );
	
	if ( (rnaCommand == RNATypeEnterBootloader) && (dataLen == 1) )
	{
		wdt_enable( WDTO_15MS ); // light the fuse
	}
	
	return 1;
}

//------------------------------------------------------------------------------
void rnaInputStream( unsigned char *data, unsigned char bytes )
{
	unsigned char* ptr = (unsigned char*)gpointer( rnaPacket );
	if ( !ptr )
	{
		return;
	}
	
	for( unsigned char i=0; i<bytes; i++ )
	{
		ptr[rnaDataReceived++] = *data++;
	}

	if ( rnaDataReceived >= rnaDataExpected )
	{
		rnaDataReceived = 0;
		if ( rnaCommand == RNATypeOledClear )
		{
			clearDisplay = true;
			goto freePacket;
		}
		else if ( rnaCommand == RNATypeSetConfigData )
		{
			menuSetConfigData( ptr, rnaFrom );
			goto freePacket;
		}
		else if ( (packetQueuePos < RNA_PACKET_QUEUE_SIZE)
				  && ((gramUsage() + rnaDataExpected) < RNA_PACKET_QUEUE_BYTE_SIZE) )
		{
			packetQueue[packetQueuePos++] = rnaPacket;
		}
		else
		{
freePacket:
			gfree( rnaPacket );
		}

		rnaPacket = 0;
	}
}

//------------------------------------------------------------------------------
// for software entry of bootloader, this is triggered with a watchdog reset
void __attribute__((OS_main)) __init()
{
	asm volatile ( "clr __zero_reg__" );

	// start the pin rising as quickly as possible
	DDRB = 0; // cheap insurance
	PORTB = 0b00000100; // turn on pullup for B2 (RNA bus)

	// if the source of the reset was a watchdog timeout, indicating a software
	// request of bootloader entry, disable the watchdog and enter the
	// bootloader
	if ( WDTCSR & (1<<WDE) )
	{
		MCUSR = 0;
		WDTCSR |= (1<<WDCE) | (1<<WDE);
		WDTCSR = 0;
		goto bootloader_jump; // waste not, want not, this saves a few bytes (ijmp take 3 instructions, goto only 1)
	}

	// The logic is a little tortured; the reason is to put the
	// bootloader jump at the very end, since it's target location
	// will vary and therefore can't be brute-force checked like the
	// rest of this code by the loader

	_delay_us(50); // give state a chance to settle

	for( unsigned int i=5000; i; i-- )
	{
		_delay_ms(1);
		// pin must be HELD low, make sure spurious RNA requests do not reset us!
		if ( PINB & 0b00000100 ) 
		{
			asm volatile ( "rjmp __ctors_end" ); // recovery not called for; return to the regularly scheduled C program
		}
	}

bootloader_jump:
	asm	volatile ("ijmp" ::"z" (OLED_BOOTLOADER_ENTRY)); // emergency reboot time
}

//------------------------------------------------------------------------------
void render()
{
	if ( frameMode == FrameConsole )
	{
		char out[SRAM_CONSOLE_LINE_BUFFER_WIDTH];
		unsigned char line = currentConsoleLine;
		unsigned char vertical = 0;
		for( unsigned char l = 0; l<SRAM_CONSOLE_LINE_BUFFER_LINES; l++ )
		{
			sramRead( SRAM_CONSOLE_LINE_BUFFER_START + (line * SRAM_CONSOLE_LINE_BUFFER_WIDTH), (unsigned char*)out, SRAM_CONSOLE_LINE_BUFFER_WIDTH );

			stringAt( out, 0, vertical, SRAM_CONSOLE_LINE_FONT, 0 );
			vertical += SRAM_CONSOLE_LINE_SPACING;
			if ( ++line >= SRAM_CONSOLE_LINE_BUFFER_LINES )
			{
				line = 0;
			}
		}
	}
	else if ( frameMode == FrameReplay )
	{
		char r[135];
		for( unsigned char i=0; i<134; i++ )
		{
			r[i] = 0;
		}
		struct PacketReplay *replay = (struct PacketReplay *)r;










		replay->millisecondsRepresented = 50;

		replay->trigger[1] = 0x0F;
		replay->eye[1] = 0xF0;
		replay->solenoid1[2] = 0xF0;
		replay->solenoid2[2] = 0x0F;



		
		unsigned char interval = replay->millisecondsRepresented / 5;
		char buf[20] = "";
		unsigned int x = 22;
		unsigned int y = 0;
		for( unsigned int i=1; i<6; i++ )
		{
			dsprintf_P( buf, PSTR("%d"), interval * i );
			stringAt( buf, x, 0, 5, 0 );
			x += 22;
		}

		stringAt_P( PSTR("T"), 0, 7, 5, 0 );
		stringAt_P( PSTR("E"), 0, 13, 5, 0 );
		stringAt_P( PSTR("1"), 0, 19, 5, 0 );
		stringAt_P( PSTR("2"), 0, 25, 5, 0 );

		for( x=8; x<128; x+=3 )
		{
			frameSetPixel( x, 8 );
		}

		for( y=8; y<32; y+=3 )
		{
			frameSetPixel( 7, y );
		}

		char lastTrigger = (replay->trigger[0] & 0x1) ? 1 : 0;
		char lastEye = (replay->eye[0] & 0x1) ? 1 : 0;
		char lastSolenoid1 = (replay->solenoid1[0] & 0x1) ? 1 : 0;
		char lastSolenoid2 = (replay->solenoid2[0] & 0x1) ? 1 : 0;
		
		for( x=9; x<128; x++ )
		{
			int pos = ((x - 9) * replay->millisecondsRepresented) / (128 - 9);
			int v1 = pos / 8;
			int v2 = 1 << (pos % 8);
			
			if ( replay->eye[v1] & v2 )
			{
				frameSetPixel( x, 9 );
			}
			else
			{
				frameSetPixel( x, 13 );
			}
			char thisEye = (replay->eye[v1] & v2) ? 1 : 0;
			if ( thisEye != lastEye )
			{
				lastEye = thisEye;
				for( y=9; y<=13; y++ )
				{
					frameSetPixel( x, y );
				}
			}
			
			if ( replay->trigger[v1] & v2 )
			{
				frameSetPixel( x, 14 );
			}
			else
			{
				frameSetPixel( x, 19 );
			}
			char thisTrigger = (replay->trigger[v1] & v2) ? 1 : 0;
			if ( thisTrigger != lastTrigger )
			{
				lastTrigger = thisTrigger;
				for( y=14; y<=19; y++ )
				{
					frameSetPixel( x, y );
				}
			}

			if ( replay->solenoid1[v1] & v2 )
			{
				frameSetPixel( x, 20 );
			}
			else
			{
				frameSetPixel( x, 25 );
			}
			char thisSolenoid1 = (replay->solenoid1[v1] & v2) ? 1 : 0;
			if ( thisSolenoid1 != lastSolenoid1 )
			{
				lastSolenoid1 = thisSolenoid1;
				for( y=20; y<=25; y++ )
				{
					frameSetPixel( x, y );
				}
			}

			if ( replay->solenoid2[v1] & v2 )
			{
				frameSetPixel( x, 26 );
			}
			else
			{
				frameSetPixel( x, 31 );
			}
			char thisSolenoid2 = (replay->solenoid2[v1] & v2) ? 1 : 0;
			if ( thisSolenoid2 != lastSolenoid2 )
			{
				lastSolenoid2 = thisSolenoid2;
				for( y=26; y<=31; y++ )
				{
					frameSetPixel( x, y );
				}
			}
		}
	}
	else if ( frameMode == FrameMenu )
	{
		menuRender();
	}
}

//------------------------------------------------------------------------------
int __attribute__((OS_main)) main()
{
	// clear RNA queue
	unsigned int i;
	unsigned char c;
	for( c =0; c<RNA_PACKET_QUEUE_SIZE; c++ )
	{
		packetQueue[c] = 0;
	}
	
	rnaInit();
	sei();

	TCCR0B = 1<<CS02 | 1<<CS00; // div/128

	sramInit();
	i2cInit();
	oledInit( 0 );

	// clear bits
	for( c = 0; c<NUMBER_OF_BIT_ENTRIES; c++ )
	{
		*(((unsigned char*)&bits) + c) = 0;
	}

	// clear SRAM
	sramStartWrite( 0 );
	for( i=0; i<8192; i++ )
	{
		sramWriteByte( 0x0 );
	}
	sramStop();

	// read in font table
	read24c512( 0xA0, 0, &numberOfFonts, 1 );
	dataBlockOrigin = ((unsigned int)numberOfFonts * sizeof(struct FontCharEntry) * 95) + 1;

	rnaDataExpected = 1; // make sure nothing triggers

	settings.invertDisplay = 0;
	settings.invertButtons = 0;
	settings.brightness = 4;
	settings.repeatDelay = 2;
	saveEEPROMConstants();

	frameMode = FrameMenu;
//	frameMode = FrameConsole;
	consolePrint( "ready" );

	displayDirty = true;

	menuSetCurrent( 0 ); // root

	for(;;)
	{
		while ( packetQueuePos ) // message pump
		{
			cli(); // must hold the lock while manipulating the heap

			// shift an entry off the queue, brute-force for now, will
			// implement a linked-list if deemed necessary
			uint8 packet = packetQueue[0];
			for( unsigned char i=1; i<packetQueuePos; i++ )
			{
				packetQueue[i - 1] = packetQueue[i];
			}
			packetQueuePos--;
			packetQueue[packetQueuePos] = 0;

			uint8 *ptr = (uint8*)gpointer( packet );

			sei();

			if ( rnaCommand == RNATypeButtonStatus )
			{
//				char buf[10];
//				dsprintf( buf, "0x%02X", *ptr );
//				consolePrint( buf ); 

				if ( frameMode == FrameMenu )
				{
					if ( menuProcessButtonState(*ptr) )
					{
						frameMode++;
					}
				}
				else if ( *ptr & ButtonBitMiddle )
				{
					frameMode++;
				}

				if ( frameMode == FrameLast )
				{
					frameMode = 0;
				}
/*
				if ( frameMode == FrameMenu )
				{
					c = RNATypeGetConfigData; // grab RNA tables from other devices
					rnaSend( 0x1, &c, 1 );
				}
*/
				if ( *ptr & ButtonBitPowerOff )
				{
					consolePrint( "POWER OFF" ); 
//					clearDisplay = true;
				}

				displayDirty = true;
			}
			else if ( rnaCommand == RNATypeReplay )
			{
				frameMode = FrameReplay;
				displayDirty = true;
			}
			else if ( rnaCommand == RNATypeOledText )
			{
				stringAt( (char*)(ptr + 3), ptr[0], ptr[1], ptr[2], 0 );
				displayBlit = true;
			}
			else if ( rnaCommand == RNATypeOledPixel )
			{
				if ( *ptr++ )
				{
					frameSetPixel( ptr[0], ptr[1] );
				}
				else
				{
					frameResetPixel( ptr[0], ptr[1] );
				}
				displayBlit = true;
			}
			else if ( rnaCommand == RNATypeOledLine )
			{
				frameLine( ptr[0], ptr[1], ptr[2], ptr[3] );
				displayBlit = true;
			}
			else if ( rnaCommand == RNATypeOledConsole )
			{
				frameMode = FrameConsole;
				ptr[31] = 0; // enforce termination
				consolePrint( (char*)ptr );
				displayDirty = true;
			}

			cli();
			gfree( packet );
			sei();
		}

		if ( TCNT0 > 200 )
		{
			if ( menuTick(TCNT0 / 8) )
			{
				displayDirty = true;
			}
			TCNT0 = 0;
		}

		if ( displayDirty )
		{
			displayDirty = false;

			frameClear();
			render();
			displayBlit = true;
		}

		if ( clearDisplay )
		{
			clearDisplay = false;
			frameClear();
			displayBlit = true;
		}

		if ( displayBlit )
		{
			displayBlit = false;
			frameBlit();
		}
	}
}

/*------------------------------------------------------------------------------*
 * Copyright: (c) 2013 by Curt Hartung avr@northarc.com
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#include <dna.h>
#include <oled.h>
#include <i2c_brute.h>
#include <24c512.h>
#include <rna.h>
#include <rna_packet.h>
#include <sram.h>

#include <util/delay.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>

//------------------------------------------------------------------------------
unsigned char rnaInputSetup( unsigned char *data, unsigned char dataLen, unsigned char from, unsigned char packetLen )
{
	if ( (*data == RNATypeEnterBootloader) && (dataLen == 1) )
	{
		wdt_enable( WDTO_15MS ); // light the fuse
	}

	return 1;
}

//------------------------------------------------------------------------------
void rnaInputStream( unsigned char *data, unsigned char bytes )
{

}

//------------------------------------------------------------------------------
// for software entry of bootloader, this is triggered with a watchdog
// reset
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

	for( unsigned int i=0xFFFF; i; i-- )
	{
		// pin must be HELD low, make sure spurious RNA requests do not reset us!
		if ( PINB & 0b00000100 ) 
		{
			asm volatile ( "rjmp __ctors_end" ); // recovery not called for; return to the regularly scheduled C program
		}
	}

bootloader_jump:
	asm	volatile ("ijmp" ::"z" (0xD00)); // emergency reboot time
}


unsigned char salt = 0;
//------------------------------------------------------------------------------
void blit()
{
	i2cStartWrite( OLED_ADDRESS );
	i2cWrite( 0x80 ); // command setup
	i2cWrite( 0x00 );
	i2cWrite( 0x80 ); // command setup
	i2cWrite( 0x10 );
	i2cWrite( 0x80 ); // command setup
	i2cWrite( 0xB0 );

	sramStartRead( 0 ); // oth location is the frame buffer

	for( unsigned char i=0; i<0xFF; i++ )
	{
		i2cWrite( 0xC0 );
		i2cWrite( sramReadByte() );
		i2cWrite( 0xC0 );
		i2cWrite( sramReadByte() );
	}

	salt++;
	
	sramStop();
	i2cWait();
	i2cStop();
}

//------------------------------------------------------------------------------
void resetPixel( unsigned char x, unsigned char y )
{
	uint address = (y >> 3) + x;
	sramAtomicWrite( address, sramAtomicRead(address) & ~(1 << (y & 0x07)) );
}

//------------------------------------------------------------------------------
void setPixel( unsigned char x, unsigned char y )
{
	uint address = (y >> 3) + x;
	sramAtomicWrite( address, sramAtomicRead(address) | (1 << (y & 0x07)) );
}

//------------------------------------------------------------------------------
void fetchEEPROMString( unsigned int string, char* buf )
{
	
}

//------------------------------------------------------------------------------
void printAt( unsigned int string, unsigned char x, unsigned char y )
{
	
}

//------------------------------------------------------------------------------
void clearFrameBuffer()
{
	sramStartWrite( 0 );
	
	for( unsigned char i=0; i<0xFF; i++ )
	{
		sramWriteByte( 0 );
		sramWriteByte( 0 );
	}

	sramStop();
}

//------------------------------------------------------------------------------
int __attribute__((OS_main)) main()
{
	sramInit();
	i2cInit();
	oledInit( 0 );
	rnaInit();
	
//	oledPowerOff();

	sei();

	clearFrameBuffer();

	unsigned char x, y;
	
	for(;;)
	{
		for( x = 0; x<128; x++ )
		{
			setPixel( x, 0 );
			setPixel( x, 31 );
		}

		for( y = 0; y<32; y++ )
		{
			setPixel( 0, y );
			setPixel( 127, y );
		}

		blit();
	}
}

/*------------------------------------------------------------------------------*
 * Copyright: (c) 2013 by Curt Hartung avr@northarc.com
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#include <dna.h>
#include <usb.h>
#include <rna.h>
#include <galloc.h>

#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include <rna_packet.h>

#include "../morlock/morlock_defs.h"

uint8 usbRNATo;
uint8 usbRNAPacket[132];
uint8 usbRNAPacketPos;
uint8 usbRNAPacketExpected;
volatile uint8 rnaPacketAvail;
uint8 usbCommand;

//------------------------------------------------------------------------------
void dnaUsbCommand( unsigned char command, unsigned char data[5] )
{
	usbCommand = ceCommandIdle;
}

//------------------------------------------------------------------------------
unsigned char dnaUsbInputSetup( unsigned char totalSize, unsigned char *data, unsigned char len )
{
	// if we are not processing a command, get one, otherwise interpret
	// as a continuation
	if ( usbCommand == ceCommandIdle )
	{
		usbCommand = data[0];

		if ( usbCommand == ceCommandRNASend )
		{
			usbRNATo = data[1];
			usbRNAPacketExpected = data[2];
			usbRNAPacketPos = 0;
			return 3;
		}
		
		return 1; // consumed command
	}

	return 0; // consumed nothing
}

//------------------------------------------------------------------------------
void dnaUsbInputStream( unsigned char *data, unsigned char len )
{
	if ( usbCommand == ceCommandIdle ) // fast-fail
	{
		return;
	}
	else if ( usbCommand == ceCommandRNASend )
	{
		for( unsigned int i=0; i<len && (usbRNAPacketPos < usbRNAPacketExpected) ; i++ )
		{
			usbRNAPacket[usbRNAPacketPos++] = data[i];
		}

		if ( usbRNAPacketPos >= usbRNAPacketExpected )
		{
			usbCommand = ceCommandIdle;
			rnaPacketAvail = 1;
		}
	}
	else
	{
		// unknown command, set to idle and hope for a recovery
		usbCommand = ceCommandIdle;
	}
}

//------------------------------------------------------------------------------
unsigned char rnaInputSetup( unsigned char *data, unsigned char dataLen, unsigned char from, unsigned char packetLen )
{
	usbRNAPacketPos = 0;
	usbRNAPacketExpected = packetLen;
	return 0;
}

//------------------------------------------------------------------------------
void rnaInputStream( unsigned char *data, unsigned char dataLen )
{
	for( unsigned char i=0; i<dataLen; i++ )
	{
		usbRNAPacket[usbRNAPacketPos++] = data[i];
	}
}

//------------------------------------------------------------------------------
int __attribute__((OS_main)) main(void)
{
	dnaUsbInit();
	enableLed();
	usbCommand = ceCommandIdle;
	usbRNAPacketExpected = 1;
	
	TCCR0B = 1<<CS01 | 1<<CS00; // set 8-bit timer prescaler to div/64
	TIMSK0 = 1<<TOIE0; // enable the interrupt
	
	rnaInit();

	sei();

	setLedOff();

	for(;;)
	{
		rnaPoll();
		
		if ( rnaPacketAvail )
		{
			setLedOn();
			rnaPacketAvail = 0;
			_delay_ms( 50 );
			while ( !rnaSend(usbRNATo, usbRNAPacket, usbRNAPacketExpected) )
			{
				_delay_ms(1);
			}
			setLedOff();
		}

		/*
		if ( usbRNAPacketPos == usbRNAPacketExpected )
		{
			usbRNAPacketPos = 0;
			if ( *usbRNAPacket == RNATypeDebugString )
			{
				usbRNAPacket[usbRNAPacketExpected] = 0;
				usbprint( (char *)usbRNAPacket + 1, usbRNAPacketExpected );
			}
		}
		*/
	}
}

//------------------------------------------------------------------------------
ISR( TIM0_OVF_vect, ISR_NOBLOCK )
{
	usbPoll(); // check for USB activity
}

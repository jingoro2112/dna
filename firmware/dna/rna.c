/* CopyRenderSettingsight: (c) 2013 by Curt Hartung
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#include "rna.h"
#include "rna_packet.h"
#include "galloc.h"

#include <util/atomic.h>

#ifndef WILL_NEVER_TALK_BACK_ON_OWN_ISR
volatile static unsigned char s_busBusy;

//------------------------------------------------------------------------------
// dynamically allocated where the heap would normall go, in a FIFO
// arrangement, If 'send' is called from within the RNA receive ISR
// (s_busBusy). They are shifted out immediately after that.
struct SendUnit
{
	unsigned char size;
	unsigned char *buf;
	unsigned char toAddress;
	unsigned char fromAddress;
	unsigned char next; // handle
};
static unsigned char s_sendUnitHead;
static unsigned char s_sendUnitTail;
#endif

// debug annunciator, timing can be tight so need to be VERY careful
// where these are placed, if enabled

//------------------------------------------------------------------------------
unsigned char rnaShiftOutByte( unsigned char data )
{
	cli();
	
	// coming into this the bus is expected to be held LOW, in
	// preparation for the first clock pulse, which is always a rising
	// edge
	unsigned char bit = 0x80;
	for( unsigned char i=8; i; i-- )
	{
		RNA_DDR ^= 1<<RNA_PIN_NUMBER; // reverse pin sense for clock pulse
		
		_delay_us( 1 ); // allow clock pulse to set up and be heard

		// assert data onto the wire
		if ( data & bit ) 
		{
			rnaSetHigh();
		}
		else
		{
			rnaSetLow();
		}

		bit >>= 1; // prepare for next

		_delay_us( 2 ); // target will be sampling at us #2, or halfway into this wait

		// if the pin is set to be high, but reading low, there is a collision
		if ( !((RNA_DDR & 1<<RNA_PIN_NUMBER) | (RNA_PIN & 1<<RNA_PIN_NUMBER)) )
		{
			return 0;
		}
	}

	rnaSetHigh(); // idle bus waiting for ack

	_delay_us( 2 );
	
	if ( rnaIsLow() ) // was there an ack?
	{
		while( rnaIsLow() ); // ack heard! remote peer can extend bus indefinitely, so we wait
		return 1;
	}

	sei();
	
	return 0;
}

//------------------------------------------------------------------------------
unsigned char rnaShiftInByte( unsigned char high )
{
	unsigned char byte = 0;
	unsigned char limit = 0;
	unsigned char bit = 0x80;

	for( unsigned char i=8; i; i-- )
	{
		if ( high )
		{
			limit = 0;
			while( rnaIsHigh() && --limit ); // wait for start (with breakout, high level might be idle bus)
		}
		else
		{
			while( rnaIsLow() ); // wait for start
		}

		_delay_us(2); // wait until the middle of Tdata

		if ( rnaIsHigh() ) // and read it
		{
			byte |= bit;
			high = 1; // bus is high, next clock pulse will be a falling edge
		}
		else
		{
			high = 0; // bus is low, next clock pulse will be a rising edge
		}

		bit >>= 1;
	}

	return byte;
}


//------------------------------------------------------------------------------
void rnaWaitForBusClear()
{
	// manchester guarantees loads of transitions, high for more than 10
	// loop cycles (even at 12mhz) is more than enough to detect idle-ness
	for ( unsigned char time = 20; time; time-- )
	{
		if ( rnaIsLow() ) // oops not idle, keep waiting
		{
			time = 20;
		}
	}
}

//------------------------------------------------------------------------------
void rnaInit()
{
	rnaSetIdle();
#ifndef RNA_POLL_DRIVEN
	rnaINTArm();
	rnaClearINT();
	rnaEnableINT();
#endif
}

//------------------------------------------------------------------------------
unsigned char rnaSendEx( unsigned char address, unsigned char fromAddress, unsigned char *data, unsigned char len )
{
	if ( address == RNA_MY_ADDRESS )
	{
		return 0; // sorta kinda undefined..
	}
	
	rnaSetActive(); // turn off pullup and assume the bus is implemented

#ifndef WILL_NEVER_TALK_BACK_ON_OWN_ISR
	if ( s_busBusy ) // being called from within the ISR: queue data for immediate transfer after its reti()
	{
		struct SendUnit *send;
		unsigned char newSend = galloc( sizeof(struct SendUnit), (void**)&send );

		if ( s_sendUnitHead )
		{
			((struct SendUnit *)gpointer(s_sendUnitTail))->next = newSend;
		}
		else
		{
			s_sendUnitHead = newSend;
		}
		s_sendUnitTail = newSend;

		send->toAddress = address;
		send->size = len;
		send->buf = data;
		send->fromAddress = fromAddress;
		send->next = 0;
	}
	else
#endif
	{
		ATOMIC_BLOCK( ATOMIC_RESTORESTATE )
		{
			for( unsigned char retries = 0; retries<5; retries++ )
			{
				rnaWaitForBusClear();

				rnaSetLow(); // attention!

				// wait a reasonable amount of time for the remote chip
				// to trigger it's interrupt, the theory is that if it
				// takes longer than this, it could take a LOT longer
				// since the chip is obviously in la-la land
				_delay_us( 11 ); 

				if ( rnaShiftOutByte(address | (fromAddress<<4)) == 1 )
				{
					// header was placed on the wire and acked

					if ( rnaShiftOutByte(len) != 1 ) // length..
					{
						len = 0;
						break;
					}

					if ( len == 0 ) // was just a probe, since we just succeeded go ahead and bug out with a positive response
					{
						len = 1;
						break;
					}

					// now just start shifting out data, if a problem is detected, scrap attempt
					for( unsigned char byte = 0; byte < len; byte++ )
					{
						if ( rnaShiftOutByte(data[byte]) != 1 ) 
						{
							len = 0;
						}
					}

					break;
				}

				// no one home or collision, back off and try again
				_delay_us( RNA_MY_ADDRESS << 4 );
			}

#ifndef RNA_POLL_DRIVEN
			rnaClearINT(); // clear the interrupt condition caused by the pin toggling
#endif
		}
	}

	rnaSetIdle();

	return len;
}

//------------------------------------------------------------------------------
#ifdef RNA_POLL_DRIVEN
void rnaPoll()
#else
ISR( RNA_ISR )
#endif
{
	if ( rnaIsHigh() ) // spurious or idle, either way get out
	{
		return;
	}

	rnaSetActive();


#ifdef RNA_POLL_DRIVEN
	ATOMIC_BLOCK( ATOMIC_RESTORESTATE )
#endif
	{
		unsigned char header = rnaShiftInByte( 0 );

		if ( (header & 0x0F) == RNA_MY_ADDRESS )
		{
			rnaSetLow(); // was meant for me, ack it!

#ifndef WILL_NEVER_TALK_BACK_ON_OWN_ISR
			// all the time in the world for bookkeeping, bus is being held
			s_busBusy = 1;
#endif

			header >>= 4; // shift off our address and just leave source

			_delay_us( 3 );

			rnaSetHigh();

			unsigned char len = rnaShiftInByte( 1 ); // get the expected length

			rnaSetLow(); // ack and HOLD for sample setup
			_delay_us( 3 );

			if ( len )
			{
				unsigned char setup = 0;
				unsigned char pos = 0;
				unsigned char index;
				unsigned char consumed;
				do
				{
					// break this up into bite-sized chunks, 8 bytes seems
					// like a good compromise, don't want to consume too
					// much memory for long messages, let the callback do
					// that if it so deems. This is not really a problem
					// since the processing overhead is fairly small
					unsigned char data[8];
					index = 0;
					for(;;)
					{
						rnaSetHigh(); // release bus
						data[index] = rnaShiftInByte( 1 ); // get some data (expect bus high intro)
						rnaSetLow(); // ack

						index++;
						pos++;

						// minimum ack setup time, theoretically this can be
						// skipped if we are going to jump to the
						// processing section (thus it would make sense to
						// put this delay AFTER the following if check)
						// but the timing is tight enough that a few extra
						// cycles would need to be added if the
						// callback is trivially implemented. So just eat the extra few cycles
						// every 8 bytes as a compromise for stability and
						// code size
						_delay_us( 3 );

						// check for completion
						if ( (index == 8) || (pos == len) ) // reach the end of the stream?
						{
							break;
						}
					}

					consumed = 0;

					if ( !setup )
					{
						setup = 1;
						consumed = rnaInputSetup( data, index, header, len );
					}

					if ( consumed <= index )
					{
						rnaInputStream( data + consumed, index - consumed );
					}

				} while( pos < len );
			}

			rnaSetHigh(); // set bus to idle while waiting to transmit

#ifndef WILL_NEVER_TALK_BACK_ON_OWN_ISR

			s_busBusy = 0;

			// now that we are not busy, see if any requests were queued up
			while( s_sendUnitHead )
			{
				struct SendUnit *send = (struct SendUnit *)gpointer( s_sendUnitHead );
				rnaSendEx( send->toAddress, send->fromAddress, send->buf, send->size );
				unsigned char next = send->next;
				gfree( s_sendUnitHead );
				s_sendUnitHead = next;
			}
			s_sendUnitTail = 0;
#endif
		}
		
		rnaSetIdle();
	}

	rnaWaitForBusClear();
	
#ifndef RNA_POLL_DRIVEN
	rnaClearINT();
#endif
}

//------------------------------------------------------------------------------
void rprint( char* string, unsigned char targetDevice )
{
	char buf[64];
	char *ptr = buf;
	*ptr++ = RNATypeOledConsole;
	for( unsigned char len=0; string[len] && len<62; len++ )
	{
		*ptr++ = string[len];
	}
	*ptr++ = 0;

	rnaSend( targetDevice, (unsigned char *)buf, ptr - buf );

	OLED_SAFE_DELAY;
}

//------------------------------------------------------------------------------
void oled_clear()
{
	unsigned char command = RNATypeOledClear;
	rnaSend( RNADeviceOLED, &command, 1 );

	OLED_SAFE_DELAY;
}

//------------------------------------------------------------------------------
void oled_pixel( char x, char y, char color )
{
	unsigned char packet[4];
	packet[0] = RNATypeOledPixel;
	packet[1] = color;
	packet[2] = x;
	packet[3] = y;
	rnaSend( RNADeviceOLED, packet, 4 );

	OLED_SAFE_DELAY;
}

//------------------------------------------------------------------------------
void oled_text( char* buf, char x, char y, char font )
{
	unsigned char packet[36];
	unsigned char *ptr = packet;
	*ptr++ = RNATypeOledText;
	*ptr++ = x;
	*ptr++ = y;
	*ptr++ = font;
	unsigned char i = 0;
	for( ; i<31 && buf[i]; i++ )
	{
		*ptr++ = buf[i];
	}
	*ptr++ = 0;
	rnaSend( RNADeviceOLED, packet, ptr - packet );

	OLED_SAFE_DELAY;
}

//------------------------------------------------------------------------------
void oled_line( char x0, char y0, char x1, char y1 )
{
	unsigned char packet[5];
	unsigned char *ptr = packet;
	*ptr++ = RNATypeOledLine;
	*ptr++ = x0;
	*ptr++ = y0;
	*ptr++ = x1;
	*ptr++ = y1;

	rnaSend( RNADeviceOLED, packet, 5 );

	OLED_SAFE_DELAY;
}


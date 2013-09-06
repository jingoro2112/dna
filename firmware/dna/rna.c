/* Copyright: (c) 2013 by Curt Hartung
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#include "rna.h"
#include "galloc.h"

#include <util/atomic.h>

volatile static unsigned char s_busBusy;

//------------------------------------------------------------------------------
// dynamically allocated where the heap would normall go, in a FIFO
// arrangement, If 'send' is called from within the RNA receive ISR
// (s_busBusy). They are shifted out immediately after that.
//
// if 
struct SendUnit
{
	unsigned char size;
	unsigned char *buf;
	unsigned char toAddress;
	unsigned char fromAddress;
	unsigned char next; // handle
};
static unsigned char s_SendUnitHead;

unsigned int rnaHeapStart;

// debug annunciator, timing can be tight so need to be VERY careful
// where these are placed!
#if 1
#undef rnaOn
#define rnaOn()
#undef rnaOff
#define rnaOff()
#endif

//------------------------------------------------------------------------------
unsigned char rnaShiftOutByte( unsigned char data, unsigned char high )
{
	// coming into this the bus is expected to be held LOW, in
	// preparation for the first clock pulse, which is always a rising
	// edge
	unsigned char collisionOccured = 0;
	unsigned char i;
	unsigned char bit = 0x80;
	for( i=8; i; i-- )
	{
		// clock is just a flip from whatever state it was left in by
		// the last data byte
		rnaOn();

		if ( high )
		{
			rnaSetLow();
		}
		else
		{
			rnaSetHigh();
		}

#if F_CPU == 12000000
		_delay_loop_1( 4 );
#elif F_CPU == 8000000
		_delay_loop_1( 2 );
#endif		

		if ( data & bit )
		{
			rnaSetHigh();
			high = 1;
			asm volatile("nop"); // make up the jump/branch opcode that causes the first case to execute two cycles faster
			asm volatile("nop");
		}
		else
		{
			rnaSetLow();
			high = 0;
		}

		bit >>= 1;
		
#if F_CPU == 12000000
		_delay_loop_1( 4 );
#elif F_CPU == 8000000
		_delay_loop_1( 2 );
#endif		

		// detect collision
		if ( high && rnaIsLow() )
		{
			collisionOccured = -1;
		}

		rnaOff();
		
#if F_CPU == 12000000
		_delay_loop_1( 4 );
#elif F_CPU == 8000000
		_delay_loop_1( 2 );
#endif
		
	}

	rnaSetHigh(); // idle bus waiting for ack

	if ( !collisionOccured ) // if there was a collision we're done
	{
#if F_CPU == 12000000
		_delay_loop_1( 11 );
#elif F_CPU == 8000000
		_delay_loop_1( 4 );
#endif		
		
		rnaOn();

		if ( rnaIsLow() ) // was there an ack?
		{
			while( rnaIsLow() ); // ack heard! remote peer can extend bus indefinitely, so we wait
			collisionOccured++; // just saving space, this will always be 1 (success)
		}
	}

	rnaOff();
	return collisionOccured;
}

//------------------------------------------------------------------------------
unsigned char rnaShiftInByte( unsigned char high )
{
	unsigned char byte = 0;
	unsigned char limit = 0;
	unsigned char i;
	unsigned char bit = 0x80;
	
	for( i=8; i; i-- )
	{
		if ( high )
		{
			while( rnaIsHigh() && --limit ); // wait for start (with breakout, high level might be idle bus)
			
#if F_CPU == 8000000
			_delay_loop_1( 3 ); // slightly less since the 'limit' dec pushes us out a little
#endif		
		}
		else
		{
			while( rnaIsLow() ); // wait for start

#if F_CPU == 8000000
			_delay_loop_1( 5 );
#endif		
		}

		rnaOn();

#if F_CPU == 12000000
		_delay_loop_1( 10 );
#endif		

		if ( rnaIsHigh() ) // and read it
		{
			rnaOff();
			byte |= bit;
			high = 1; // bus is high, next clock pulse will be a falling edge
		}
		else
		{
			rnaOff();
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
	unsigned char time;
	for ( time = 20; time; time-- )
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
	// turn on pullup
	rnaSetHigh();
	RNA_PORT |= (1<<RNA_PIN_NUMBER);
	
	// assert low
	rnaSetLow();
	RNA_PORT &= ~(1<<RNA_PIN_NUMBER);

	rnaSetHigh(); // idle

	s_sendUnitHead = 0;
	s_busBusy = 0;

	rnaHeapStart = __heap_start;

	rnaINTArm();
	rnaClearINT();
	rnaEnableINT();
}

//------------------------------------------------------------------------------
static unsigned char attention( unsigned char header )
{
	rnaWaitForBusClear();

	rnaSetLow();
	_delay_us( 11 );
	
	return rnaShiftOutByte( header, 0 );
}

//------------------------------------------------------------------------------
unsigned char rnaProbe( unsigned char address )
{
	if ( s_busBusy ) // cannot call this from within an ISR, fail gracefully
	{
		return 0;
	}

	ATOMIC_BLOCK( ATOMIC_RESTORESTATE )
	{
		for(;;)
		{
			cli();

			unsigned char ret = attention( address | (RNA_MY_ADDRESS<<4) );
			if ( ret == 1 )
			{
				rnaShiftOutByte( 0, 1 ); // send zero-length transmission
				return 1;
			}
			else if ( ret == 0 ) // no one home
			{
				return 0;
			}

			// otherwise there might have been a collision, try again
			_delay_us( RNA_MY_ADDRESS << 3 );
		}

		rnaSetHigh();

#ifndef RNA_POLL_DRIVEN
		rnaClearINT();
#endif
	}

	rnaOff();
	return 1;
}

//------------------------------------------------------------------------------
void rnaSendEx( unsigned char address, unsigned char fromAddress, unsigned char *data, unsigned char len )
{
	if ( s_busBusy ) // being called from within the ISR: queue data for immediate transfer after its reti()
	{
		struct SendUnit *send = ((struct SendUnit *)rnaHeapStart) + s_sendQueueSize;
		send->toAddress = address;
		send->size = len;
		send->buf = data;
		send->fromAddress = fromAddress;
		s_sendQueueSize++;
		return;
	}

	ATOMIC_BLOCK( ATOMIC_RESTORESTATE )
	{
		for( unsigned char retries=0; retries<5; retries++ )
		{
			unsigned char ret = attention( address | (fromAddress<<4) );

			if ( ret == 1 )
			{
				// header was placed on the wire and acked

				if ( rnaShiftOutByte(len, 1) != 1 ) // length..
				{
					break; // not sure how to continue, chip died mid-transmission?
				}

				// now just start shifting out data, if a problem is detected, scrap attempt
				unsigned char byte;
				for( byte = 0; byte < len; byte++ )
				{
					if ( rnaShiftOutByte(data[byte], 1) != 1 ) 
					{
						break;
					}
				}

				break;
			}

			// no one home or collision, back off and try again
			_delay_us( RNA_MY_ADDRESS << 3 );
		}

		rnaSetHigh();
		rnaOff();

#ifndef RNA_POLL_DRIVEN
		rnaClearINT(); // clear the interrupt condition caused by the pin toggling
#endif

	}
}

//------------------------------------------------------------------------------
// send a message to a device as if we are the virtual system device,
// use with caution!
void rnaSendSystem( unsigned char address, unsigned char *data, unsigned char len )
{
	rnaSendEx( address, 0, data, len );
}

//------------------------------------------------------------------------------
void rnaSend( unsigned char address, unsigned char *data, unsigned char len )
{
	rnaSendEx( address, RNA_MY_ADDRESS, data, len );
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

#ifdef RNA_POLL_DRIVEN
	cli();
#endif
	
	rnaOn();

	unsigned char header = rnaShiftInByte( 0 );

	if ( (header & 0x0F) != RNA_MY_ADDRESS )
	{
		// not meant for me, wait for an idle bus so we do not try and
		// interpret garbage on the next interrupt
		rnaWaitForBusClear();
	}
	else
	{
		rnaSetLow(); // was meant for me, ack it!

		// all the time in the world for bookkeeping, bus is being held
		s_busBusy = 1;

		rnaOn();
		
		header >>= 4; // shift off our address and just leave source

#if F_CPU == 12000000
		_delay_loop_1( 9 );
#elif F_CPU == 8000000
		_delay_loop_1( 6 );
#endif		

		rnaOff();
		rnaSetHigh();

		unsigned char len = rnaShiftInByte( 1 ); // get the expected length

		rnaSetLow(); // ack and HOLD for sample setup

#if F_CPU == 12000000
		_delay_loop_1( 16 );
#elif F_CPU == 8000000
		_delay_loop_1( 8 );
#endif

		if ( len )
		{
			unsigned char index = 0;
			unsigned char setup = 0;
			unsigned char pos = 0;
			do
			{
				// break this up into bite-sized chunks, 8 bytes seems
				// like a good compromise, don't want to consume to
				// omuch memory for long messages ,let the callback do
				// that if it so deems. This is not really a problem
				// since the processing overhead is fairly small
				unsigned char data[8];

				for( index=0; index<8; index++ )
				{
					rnaSetHigh(); // release bus

					data[index] = rnaShiftInByte( 1 ); // get some data (expect bus high intro)

					rnaSetLow(); // ack and hold/extend for (potential) processing

#if F_CPU == 12000000
					_delay_loop_1( 11 );
#elif F_CPU == 8000000
					_delay_loop_1( 4 );
#endif

					if ( ++pos >= len ) // reach the end of the stream?
					{
						break;
					}
				}

				rnaOn();
				
				if ( setup )
				{
					rnaInputStream( data, index ); // setup has been called, this data better be expected
				}
				else
				{
					unsigned char consumed;
#ifndef DNA
					if ( !header )
					{
						// came from the 'zeroth' device. only one command is valid: jump to bootloader
						asm	volatile ("ijmp" ::"z" (RNA_BOOTLOADER_ENTRY));
					}
#endif
					
					consumed = rnaInputSetup( data, header, index );
					if ( consumed < index )
					{
						rnaInputStream( data + consumed, index - consumed );	
					}
					
					setup = 1;
				}

				rnaOff();
				
			} while( pos < len );
		}
		
		rnaSetHigh(); // bus idle

		s_busBusy = 0;

		unsigned char i;
		for( i = 0; i<s_sendQueueSize; i++ )
		{
			struct SendUnit *send = ((struct SendUnit *)rnaHeapStart) + i;
			rnaSend( send->toAddress, send->buf, send->size );
		}
		s_sendQueueSize = 0;
	}

	rnaOff();

#ifdef RNA_POLL_DRIVEN
	reti();
#else
	rnaClearINT();
#endif
}

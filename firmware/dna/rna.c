/* Copyright: (c) 2013 by Curt Hartung
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#include "rna.h"
#include "galloc.h"

#include <util/atomic.h>

#if (F_CPU != 12000000) && (F_CPU != 8000000)
#error unsupported F_CPU for RNA driver, must be 8 or 12 mHz
#endif		

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

// debug annunciator, timing can be tight so need to be VERY careful
// where these are placed, if enabled

//------------------------------------------------------------------------------
unsigned char rnaShiftOutByte( unsigned char data, unsigned char high )
{
	// coming into this the bus is expected to be held LOW, in
	// preparation for the first clock pulse, which is always a rising
	// edge
	unsigned char collisionOccured = 0;
	unsigned char bit = 0x80;
	for( unsigned char i=8; i; i-- )
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
	rnaINTArm();
	rnaClearINT();
	rnaEnableINT();
}

//------------------------------------------------------------------------------
unsigned char rnaSendEx( unsigned char address, unsigned char fromAddress, unsigned char *data, unsigned char len )
{
	rnaSetActive();

	if ( s_busBusy ) // being called from within the ISR: queue data for immediate transfer after its reti()
	{
		unsigned char newSend = galloc( sizeof(struct SendUnit) );
	
		if ( s_sendUnitHead )
		{
			// find the end of the list
			struct SendUnit *S;

			// in-place traverse
			for( S = (struct SendUnit *)gpointer(s_sendUnitHead);
				 S->next;
				 S = (struct SendUnit *)gpointer(S->next) );
			
			S->next = newSend;
		}
		else
		{
			s_sendUnitHead = newSend;
		}

		struct SendUnit *send = (struct SendUnit *)gpointer( newSend );
		
		send->toAddress = address;
		send->size = len;
		send->buf = data;
		send->fromAddress = fromAddress;
		send->next = 0;
	}
	else
	{
		ATOMIC_BLOCK( ATOMIC_RESTORESTATE )
		{
			for( unsigned char retries = 0; retries<5; retries++ )
			{
				rnaWaitForBusClear();

				rnaSetLow(); // attention!
				_delay_us( 11 ); // wait a reasonable amount of time for the remote chip to trigger its interrupt, more than this and it might be WAY more than this

				if ( rnaShiftOutByte(address | (fromAddress<<4), 0) == 1 )
				{
					// header was placed on the wire and acked

					if ( rnaShiftOutByte(len, 1) != 1 ) // length..
					{
						len = 0;
						break;
					}

					// now just start shifting out data, if a problem is detected, scrap attempt
					for( unsigned char byte = 0; byte < len; byte++ )
					{
						if ( rnaShiftOutByte(data[byte], 1) != 1 ) 
						{
							len = 0;
						}
					}

					break;
				}

				// no one home or collision, back off and try again
				_delay_us( RNA_MY_ADDRESS << 3 );
			}

			rnaOff();

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

		// now that we are not busy, see if any requests were queued up
		while( s_sendUnitHead )
		{
			struct SendUnit *send = (struct SendUnit *)gpointer( s_sendUnitHead );
			rnaSendEx( send->toAddress, send->fromAddress, send->buf, send->size );
			unsigned char next = send->next;
			gfree( s_sendUnitHead );
			s_sendUnitHead = next;
		}
	}

	rnaOff();

	rnaSetIdle();

#ifdef RNA_POLL_DRIVEN
	reti();
#else
	rnaClearINT();
#endif
}

/*
//------------------------------------------------------------------------------
void rnaDebugMessage( char* message )
{
	unsigned char len = 0;
	for( ; message[len]; len++ );
	len++;

	unsigned char hndl = galloc( len );
	char* msg = gpointer( hndl );
	msg
	

	unsigned char newSend = galloc( sizeof(struct SendUnit) );

	if ( RNA_MY_ADDRESS != 0x1 ) // not the master
	{
		while( 
		rnaSendEx( 0x1, RNA_MY_ADDRESS, message,  )
	}
}
*/
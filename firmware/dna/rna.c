#include "rna.h"

static unsigned char s_myAddress;

volatile static unsigned char s_busBusy;

//------------------------------------------------------------------------------
struct SendUnit
{
	unsigned char size;
	unsigned char toAddress;
	unsigned char *buf;
};
static unsigned char s_sendQueueSize;
extern unsigned int __heap_start;
unsigned int rnaHeapStart;

#if 1
#if defined ( _AVR_IOMX8_H_ )
	#define On()  (PORTD |= 0b01000000)
	#define Off() (PORTD &= 0b10111111)
#else
	#define On()  (PORTA |= 0b10000000)
	#define Off() (PORTA &= 0b01111111)
#endif
#else
#define On()
#define Off()
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
		On();

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

		Off();
		
#if F_CPU == 12000000
		_delay_loop_1( 4 );
#elif F_CPU == 8000000
		_delay_loop_1( 2 );
#endif
		
	}

	rnaSetHigh(); // set bus idle (might be very temporary)

	if ( !collisionOccured ) // if there was a collision we're done
	{

#if F_CPU == 12000000
		_delay_loop_1( 11 );
#elif F_CPU == 8000000
		_delay_loop_1( 2 ); // !!!!!!!!!!!!!!!!!!!! huh?
#endif		
		
		On();

		if ( rnaIsLow() )
		{
			while( rnaIsLow() ); // ack heard, and waiting
			
			collisionOccured++; // just saving space, this will always aoutput a '1' success from rnaShiftOutByte
		}
	}

	Off();
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

		On();

#if F_CPU == 12000000
		_delay_loop_1( 10 );
#endif		

		if ( rnaIsHigh() ) // and read it
		{
			Off();
			byte |= bit;
			high = 1; // bus is high, next clock pulse will be a falling edge
		}
		else
		{
			Off();
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
void rnaInit( unsigned char address )
{
	// turn on pullup
	rnaSetHigh();
	RNA_PORT |= (1<<RNA_PIN_NUMBER);
	
	// assert low
	rnaSetLow();
	RNA_PORT &= ~(1<<RNA_PIN_NUMBER);

	rnaSetHigh(); // idle

	s_myAddress = address;

	s_sendQueueSize = 0;
	s_busBusy = 0;

	rnaHeapStart = __heap_start;

	rnaINTArm();
	rnaEnableINT();
}

//------------------------------------------------------------------------------
static unsigned char attention( unsigned char address )
{
	rnaWaitForBusClear();

	unsigned char header = address | (s_myAddress<<4); // from -> to

	rnaSetLow();
	_delay_us( 11 );
	
	return rnaShiftOutByte( header, 0 );
}

//------------------------------------------------------------------------------
unsigned char rnaProbe( unsigned char address )
{
	if ( s_busBusy )
	{
		return 0;
	}

	for(;;)
	{
		unsigned char ret = attention( address );
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
		volatile unsigned char time = s_myAddress << 3;
		for( ; time; time-- );
	}

	rnaSetHigh();

#ifndef RNA_POLL_DRIVEN
	rnaClearINT();
#endif

	sei();
	Off();
}

//------------------------------------------------------------------------------
// send a message to a device as if we are the virtual system device,
// use with caution!
void rnaSendSystem( unsigned char address, unsigned char *data, unsigned char len )
{
	cli();
	unsigned char t = s_myAddress;
	s_myAddress = 0;
	rnaSend( address, data, len ); // this call will sei() before returning
	s_myAddress = t;
}

//------------------------------------------------------------------------------
void rnaSend( unsigned char address, unsigned char *data, unsigned char len )
{
	if ( s_busBusy ) // being called from within the ISR: queue data for immediate transfer
	{
		struct SendUnit *send = ((struct SendUnit *)rnaHeapStart) + s_sendQueueSize;
		send->toAddress = address;
		send->size = len;
		send->buf = data;
		s_sendQueueSize++;
		return;
	}

	cli();

	for(;;)
	{
		unsigned char ret = attention( address );
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
		else if ( ret == 0 ) // no one home
		{
			break;
		}

		// -1, collision, back off and try again
		volatile unsigned char time;
		for( time = s_myAddress << 3; time; time-- );
	}

	rnaSetHigh();
	Off();

#ifndef RNA_POLL_DRIVEN
	rnaClearINT(); // clear the interrupt condition caused by the pin toggling
#endif

	reti(); // guarantee a return, in case what we just sent is about to interrupt us with a reply
}

//------------------------------------------------------------------------------
// commands that come from the "system" level device are handled
// separatrely to insulate the user from needing to handle it in
// user-level code
unsigned char rnaSystemInputSetup( unsigned char *data, unsigned char len )
{
//	RNACommandCodePage
	return 0;
}

//------------------------------------------------------------------------------
void rnaSystemInputStream( unsigned char *data, unsigned char bytes )
{
	
}
	
//------------------------------------------------------------------------------
#ifndef RNA_POLL_DRIVEN
#if defined(PROTO88)
ISR( INT1_vect )
#elif defined(DNAPROTO)
ISR( PCINT0_vect )
#elif defined(DNA)
ISR( PCINT1_vect )
#elif defined(OLED)
ISR( INT0_vect )
#endif
#else
void rnaPoll()
#endif
{
	if ( rnaIsHigh() ) // spurious or idle, either way get out
	{
		return;
	}

#ifdef RNA_POLL_DRIVEN
	cli();
#endif
	
	On();

	unsigned char header = rnaShiftInByte( 0 );

	if ( (header & 0x0F) != s_myAddress )
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

		On();
		
		header >>= 4; // shift off our address and just leave source

#if F_CPU == 12000000
		_delay_loop_1( 9 );
#elif F_CPU == 8000000
		_delay_loop_1( 6 );
#endif		

		Off();
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

				On();
				
				if ( setup )
				{
					if ( header )
					{
						rnaInputStream( data, index ); // setup has been called, this data better be expected
					}
					else
					{
						rnaSystemInputStream( data, index ); // system-level commands (coming from virtual source 0x0)
					}
				}
				else
				{
					unsigned char consumed;
					if ( header )
					{
						consumed = rnaInputSetup( data, header, index );
						if ( consumed < index )
						{
							rnaInputStream( data + consumed, index - consumed );	
						}
					}
					else
					{
						consumed = rnaSystemInputSetup( data, index );
						if ( consumed < index )
						{
							rnaSystemInputStream( data + consumed, index - consumed );	
						}
					}
					
					setup = 1;
				}

				Off();
				
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

	Off();

#ifdef RNA_POLL_DRIVEN
	reti();
#else
	rnaClearINT();
#endif
}

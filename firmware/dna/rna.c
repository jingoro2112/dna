#include "rna.h"

#if defined ( _AVR_IOMX8_H_ )
#define On()  (PORTD |= 0b01000000)
#define Off() (PORTD &= 0b10111111)
#else
#define On() (DDRA |= 0b10000000); (PORTA &= 0b01111111)
#define Off() (DDRA &= 0b01111111)
#endif

static unsigned char s_address;

volatile static unsigned char s_busBusy;
volatile static unsigned char s_sendSize;
volatile static unsigned char s_sendHeader;
volatile static unsigned char *s_sendBuf;

#if defined ( _AVR_IOMX8_H_ )
#define On()  (PORTD |= 0b01000000)
#define Off() (PORTD &= 0b10111111)
#else
#define On() (DDRA |= 0b10000000); (PORTA &= 0b01111111)
#define Off() (DDRA &= 0b01111111)
#endif

//------------------------------------------------------------------------------
unsigned char rnaShiftOutByte( unsigned char data )
{
	volatile unsigned char time;
	rnaSetLow();  // start

	On();
	if ( data & 0b10000000 )
	{
		rnaSetHigh();

		for( time = RNA_T_CONST_HALF_SETUP; time; time-- );

		if ( rnaIsLow() ) // collision detection
		{
			goto collision;
		}
	}
	else
	{
		rnaSetLow();

		for( time = RNA_T_CONST_HALF_SETUP; time; time-- );
		asm volatile("nop"); // burn the same time as the collision detector
		asm volatile("nop");
	}
	for( time = RNA_T_CONST_HALF_SETUP + RNA_T_CONST_SETUP_EXTEND; time; time-- );
	Off();

	On();
	if ( data & 0b01000000 )
	{
		rnaSetHigh();

		for( time = RNA_T_CONST_HALF_SETUP; time; time-- );

		if ( rnaIsLow() ) // collision detection
		{
			goto collision;
		}

	}
	else
	{
		rnaSetLow();

		for( time = RNA_T_CONST_HALF_SETUP; time; time-- );
		asm volatile("nop"); // burn the same time as the collision detector
		asm volatile("nop");
	}
	for( time = RNA_T_CONST_HALF_SETUP + RNA_T_CONST_SETUP_EXTEND; time; time-- );
	Off();

	On();
	if ( data & 0b00100000 )
	{
		rnaSetHigh();

		for( time = RNA_T_CONST_HALF_SETUP; time; time-- );

		if ( rnaIsLow() ) // collision detection
		{
			goto collision;
		}

	}
	else
	{
		rnaSetLow();

		for( time = RNA_T_CONST_HALF_SETUP; time; time-- );
		asm volatile("nop"); // burn the same time as the collision detector
		asm volatile("nop");
	}
	for( time = RNA_T_CONST_HALF_SETUP + RNA_T_CONST_SETUP_EXTEND; time; time-- );
	Off();

	On();
	if ( data & 0b00010000 )
	{
		rnaSetHigh();

		for( time = RNA_T_CONST_HALF_SETUP; time; time-- );

		if ( rnaIsLow() ) // collision detection
		{
			goto collision;
		}

	}
	else
	{
		rnaSetLow();

		for( time = RNA_T_CONST_HALF_SETUP; time; time-- );
		asm volatile("nop"); // burn the same time as the collision detector
		asm volatile("nop");
	}
	for( time = RNA_T_CONST_HALF_SETUP + RNA_T_CONST_SETUP_EXTEND; time; time-- );
	Off();

	On();
	if ( data & 0b00001000 )
	{
		rnaSetHigh();

		for( time = RNA_T_CONST_HALF_SETUP; time; time-- );

		if ( rnaIsLow() ) // collision detection
		{
			goto collision;
		}

	}
	else
	{
		rnaSetLow();

		for( time = RNA_T_CONST_HALF_SETUP; time; time-- );
		asm volatile("nop"); // burn the same time as the collision detector
		asm volatile("nop");
	}
	for( time = RNA_T_CONST_HALF_SETUP + RNA_T_CONST_SETUP_EXTEND; time; time-- );
	Off();

	On();
	if ( data & 0b00000100 )
	{
		rnaSetHigh();

		for( time = RNA_T_CONST_HALF_SETUP; time; time-- );

		if ( rnaIsLow() ) // collision detection
		{
			goto collision;
		}

	}
	else
	{
		rnaSetLow();

		for( time = RNA_T_CONST_HALF_SETUP; time; time-- );
		asm volatile("nop"); // burn the same time as the collision detector
		asm volatile("nop");
	}
	for( time = RNA_T_CONST_HALF_SETUP + RNA_T_CONST_SETUP_EXTEND; time; time-- );
	Off();

	On();
	if ( data & 0b00000010 )
	{
		rnaSetHigh();
		for( time = RNA_T_CONST_HALF_SETUP; time; time-- );

		if ( rnaIsLow() ) // collision detection
		{
			goto collision;
		}
	}
	else
	{
		rnaSetLow();
		for( time = RNA_T_CONST_HALF_SETUP; time; time-- );
		asm volatile("nop"); // burn the same time as the collision detector
		asm volatile("nop");
	}
	for( time = RNA_T_CONST_HALF_SETUP + RNA_T_CONST_SETUP_EXTEND; time; time-- );
	Off();

	On();
	if ( data & 0b00000001 )
	{
		rnaSetHigh();

		for( time = RNA_T_CONST_HALF_SETUP; time; time-- );

		if ( rnaIsLow() ) // collision detection
		{
			goto collision;
		}
	}
	else
	{
		rnaSetLow();

		for( time = RNA_T_CONST_HALF_SETUP; time; time-- );
		asm volatile("nop"); // burn the same time as the collision detector
		asm volatile("nop");
	}
	for( time = RNA_T_CONST_HALF_SETUP + RNA_T_CONST_SETUP_EXTEND; time; time-- );
	Off();

	rnaSetHigh();
	for( time = RNA_T_CONST_ACK_WAIT; time; time-- );

	On();
	if ( rnaIsLow() )
	{
		while( rnaIsLow() ); // a low holds the bus forever
		return 1;
	}
	else
	{
		return 0;
	}
	Off();

collision:
	return -1;
}

//------------------------------------------------------------------------------
unsigned char rnaShiftInByte()
{
	volatile unsigned char time;

	Off();
	unsigned d = 0;
	while( rnaIsLow() ); // wait for rise (if necessary)
	while( rnaIsHigh() );

	On();
	for( time = RNA_T_CONST_SAMPLE; time; time-- );
	if ( rnaIsHigh() )
	{
		d |= 0b10000000;
	}
	else
	{
		asm volatile("nop");
		asm volatile("nop");
	}
	asm volatile("nop");
	asm volatile("nop");
	Off();
	for( time = RNA_T_CONST_SAMPLE + RNA_T_CONST_SAMPLE_REMAINDER; time; time-- );

	On();
	if ( rnaIsHigh() )
	{
		d |= 0b01000000;
	}
	else
	{
		asm volatile("nop");
		asm volatile("nop");
	}
	asm volatile("nop");
	asm volatile("nop");
	Off();
	for( time = RNA_T_CONST_SAMPLE + RNA_T_CONST_SAMPLE_REMAINDER; time; time-- );

	On();
	if ( rnaIsHigh() )
	{
		d |= 0b00100000;
	}
	else
	{
		asm volatile("nop");
		asm volatile("nop");
	}
	asm volatile("nop");
	asm volatile("nop");
	Off();
	for( time = RNA_T_CONST_SAMPLE + RNA_T_CONST_SAMPLE_REMAINDER; time; time-- );

	On();
	if ( rnaIsHigh() )
	{
		d |= 0b00010000;
	}
	else
	{
		asm volatile("nop");
		asm volatile("nop");
	}
	asm volatile("nop");
	asm volatile("nop");
	Off();
	for( time = RNA_T_CONST_SAMPLE + RNA_T_CONST_SAMPLE_REMAINDER; time; time-- );

	On();
	if ( rnaIsHigh() )
	{
		d |= 0b00001000;
	}
	else
	{
		asm volatile("nop");
		asm volatile("nop");
	}
	asm volatile("nop");
	asm volatile("nop");
	Off();
	for( time = RNA_T_CONST_SAMPLE + RNA_T_CONST_SAMPLE_REMAINDER; time; time-- );

	On();
	if ( rnaIsHigh() )
	{
		d |= 0b00000100;
	}
	else
	{
		asm volatile("nop");
		asm volatile("nop");
	}
	asm volatile("nop");
	asm volatile("nop");
	Off();
	for( time = RNA_T_CONST_SAMPLE + RNA_T_CONST_SAMPLE_REMAINDER; time; time-- );

	On();
	if ( rnaIsHigh() )
	{
		d |= 0b00000010;
	}
	else
	{
		asm volatile("nop");
		asm volatile("nop");
	}
	asm volatile("nop");
	asm volatile("nop");
	Off();
	for( time = RNA_T_CONST_SAMPLE + RNA_T_CONST_SAMPLE_REMAINDER; time; time-- );

	On();
	if ( rnaIsHigh() )
	{
		d |= 0b00000001;
	}
	else
	{
		asm volatile("nop");
		asm volatile("nop");
	}
	Off();
	return d;
}

//------------------------------------------------------------------------------
void rnaWaitForBusClear()
{
	// wait for bus to go idle, longest it can be high (non idle) is a string
	// of 1's in data, which is around 1.2us
	volatile unsigned char time;
	for ( time = 30; time; time++ ) // more than enough, even on 12mHz
	{
		if ( rnaIsLow() ) // oops not idle, try again
		{
			time = 0;
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

	s_address = address;

	s_busBusy = 0;
	s_sendSize = 0;

#if defined ( _AVR_IOMX8_H_ )
	rnaINT1ToFallingEdge();
	rnaEnableINT1();
#else
	rnaPCINTArm();
	rnaEnablePCINT();
#endif
}

//------------------------------------------------------------------------------
unsigned char rnaProbe( unsigned char address )
{
	if ( s_busBusy )
	{
		return 0;
	}

	rnaWaitForBusClear();
	
	unsigned char header = address | (s_address<<4); // from -> to
	for(;;)
	{
		cli();

		// assert interrupt
		rnaSetLow();
		volatile unsigned char time;
		for( time = RNA_T_CONST_HALF_SETUP*3; time; time-- );

		// 
		rnaSetHigh();
		for( time = RNA_T_CONST_SAMPLE + RNA_T_CONST_SAMPLE_REMAINDER; time; time-- );

		unsigned char ret = rnaShiftOutByte( header );
		if ( ret == 1 )
		{
			rnaShiftOutByte(0); // length zero
			return 1;
		}
		else if ( ret == 0 ) // no one home
		{
			return 0;
		}

		// otherwise there might have been a collision, try again
		
		// clear the interrupt conditions we just caused
#if defined ( _AVR_IOMX8_H_ )
		rnaClearINT1();
#else
		rnaClearPCINT();
#endif

		// else must have been a collision, back off and try again
		sei();
		for( time = (RNA_T_CONST_HALF_SETUP*s_address); time; time-- );
	}

#if defined ( _AVR_IOMX8_H_ )
	rnaClearINT1();
#else
	rnaClearPCINT();
#endif

	sei();
}

//------------------------------------------------------------------------------
void rnaSend( unsigned char address, unsigned char *data, unsigned char len )
{
	s_sendHeader = address | (s_address<<4); // from -> to

	if ( s_busBusy )
	{
		s_sendHeader = len;
		s_sendBuf = data;
		return;
	}

	rnaWaitForBusClear();

	volatile unsigned char time;
	unsigned char ret;

	for(;;)
	{
		cli();

		rnaSetLow();
		for( time = RNA_T_CONST_HALF_SETUP*3; time; time-- );
		
		rnaSetHigh();
		for( time = RNA_T_CONST_SAMPLE + RNA_T_CONST_SAMPLE_REMAINDER; time; time-- );

		ret = rnaShiftOutByte( s_sendHeader );

		if ( ret == 1 )
		{
			// success! shift out the payload
			
			if ( rnaShiftOutByte(len) != 1 ) // length..
			{
				return;
			}

			for( time = 0; time < len; time++ ) // data
			{
				if ( rnaShiftOutByte(data[time]) != 1 ) // keep going until there is a problem
				{
					return;
				}
			}

			return;
		}
		else if ( ret == 0 ) // no one home
		{
			return;
		}

		// clear the interrupt conditions we just caused
#if defined ( _AVR_IOMX8_H_ )
		rnaClearINT1();
#else
		rnaClearPCINT();
#endif

		// else must have been a collision, back off and try again
		sei();
		for( time = (RNA_T_CONST_HALF_SETUP*s_address); time; time-- );
	}

	// clear the interrupt conditions we just caused
#if defined ( _AVR_IOMX8_H_ )
	rnaClearINT1();
#else
	rnaClearPCINT();
#endif

	sei();
}

//------------------------------------------------------------------------------
#if defined ( _AVR_IOMX8_H_ )
ISR( INT1_vect )
#else
//ISR( PCINT1_vect ) // /RESET
ISR( PCINT0_vect ) // A2
#endif
{
	if ( rnaIsHigh() ) // spurious
	{
		return;
	}

	Off();

	volatile unsigned char time;
	unsigned char data[8];
	unsigned char len;
	unsigned char pos = 0;
	unsigned char from;

	cli();

	while( rnaIsLow() ); // wait for the attention signal to lapse

	from = rnaShiftInByte();
	On();

	if ( (from & 0x0F) != s_address )
	{
		rnaWaitForBusClear();
	}
	else
	{
		rnaSetLow(); // hurry up and ACK! this was meant for me
		for( time = RNA_T_CONST_ACK_SETUP; time; time-- );
		rnaSetHigh();

		Off();
		
		len = rnaShiftInByte(); // get the expected length

		if ( len )
		{
			rnaSetLow();
			for( time = RNA_T_CONST_ACK_SETUP; time; time-- );

			unsigned char index = 1;
			unsigned char setup = 0;
			do
			{
				for( index=0; index<8; index++ )
				{
					rnaSetHigh();
					data[index] = rnaShiftInByte();

					rnaSetLow(); // ack and HOLD until the data has been delivered

					for( time = RNA_T_CONST_ACK_SETUP - 1; time; time-- );

					if ( ++pos >= len )
					{
						break;
					}
				}

				if ( setup )
				{
					rnaInputStream( data, index ); // setup has been called, this data better be expected
				}
				else
				{
					unsigned char consumed = rnaInputSetup( data, from>>4, index );
					if ( consumed < index )
					{
						rnaInputStream( data + consumed, index - consumed );	
					}
					setup = 1;
				}

				rnaSetHigh();

			} while( pos < len );
		}
	}

#if defined ( _AVR_IOMX8_H_ )
	rnaClearINT1();
#else
	rnaClearPCINT();
#endif
	
	sei();
}

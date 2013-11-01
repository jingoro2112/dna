#include <rna.h>
/*------------------------------------------------------------------------------*
 * Copyright: (c) 2013 by Curt Hartung avr@northarc.com
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#include "telemetry.h"

#include <dna.h>
#include <rna_packet.h>

#include <avr/wdt.h>
#include <util/delay.h>

#include <stdio.h>
#include <stdlib.h>

//------------------------------------------------------------------------------
unsigned char rnaInputSetup( unsigned char *data, unsigned char dataLen, unsigned char from, unsigned char totalLen )
{
	if ( *data == RNATypeEnterBootloader && (dataLen == 1) )
	{
		wdt_enable( WDTO_15MS ); // light the fuse
	}
	return 0;
}

//------------------------------------------------------------------------------
void rnaInputStream( unsigned char *data, unsigned char dataLen )
{

}

#define PURPLE_OFF (PORTC |= 0b00100000)
#define PURPLE_ON (PORTC &= ~0b00100000)

#define TY2_OFF (PORTC |= 0b00010000)
#define TY2_ON (PORTC &= ~0b00010000)
#define TY1_OFF (PORTD |= 0b00000010)
#define TY1_ON (PORTD &= ~0b00000010)
#define TR_OFF (PORTD |= 0b00100000)
#define TR_ON (PORTD &= ~0b00100000)
#define TG_OFF (PORTB |= 0b10000000)
#define TG_ON (PORTB &= ~0b10000000)

#define Y1_OFF (PORTB |= 0b01000000)
#define Y1_ON (PORTB &= ~0b01000000)
#define Y2_OFF (PORTD |= 0b00010000)
#define Y2_ON (PORTD &= ~0b00010000)
#define Y3_OFF (PORTC |= 0b00000001)
#define Y3_ON (PORTC &= ~0b00000001)

#define G1_OFF (PORTB |= 0b00001000)
#define G1_ON (PORTB &= ~0b00001000)
#define G2_OFF (PORTD |= 0b00000001)
#define G2_ON (PORTD &= ~0b00000001)
#define G3_OFF (PORTB |= 0b00010000)
#define G3_ON (PORTB &= ~0b00010000)
#define G4_OFF (PORTC |= 0b00000010)
#define G4_ON (PORTC &= ~0b00000010)

#define R1_OFF (PORTD |= 0b00001000)
#define R1_ON (PORTD &= ~0b00001000)
#define R2_OFF (PORTC |= 0b00000100)
#define R2_ON (PORTC &= ~0b00000100)
#define R3_OFF (PORTB |= 0b00100000)
#define R3_ON (PORTB &= ~0b00100000)
#define R4_OFF (PORTC |= 0b00001000)
#define R4_ON (PORTC &= ~0b00001000)

//------------------------------------------------------------------------------
enum
{
	PURPLE = 0,
	TY1,
	TR,
	TG,
	TY2,
	G1,
	G2,
	G3,
	G4,
	R1,
	R2,
	R3,
	R4,
	Y1,
	Y2,
	Y3,
};

volatile char intensity[16];

//------------------------------------------------------------------------------
void compute( int cycles )
{
	unsigned char state = 0;
	char rate = 5;
	for( int y=0; y<cycles; y++ )
	{
		state++;
		
		if ( !(state % 10) )
		{
			intensity[TY1] = rand() & 0x0F;
			intensity[TY2] = rand() & 0x0F;
			intensity[TR] = (rand() & 0x1) ? 0xFF : 0;
			intensity[TG] = (rand() & 0x1) ? 0xFF : 0;
		}

		intensity[G1] = rand() & 0xFF;
		intensity[G2] = rand() & 0xFF;
		intensity[G3] = rand() & 0xFF;
		intensity[G4] = rand() & 0xFF;
		intensity[Y1] = rand() & 0xFF;
		intensity[Y2] = rand() & 0xFF;

		if ( intensity[PURPLE] > 100 )
		{
			rate = -5;
		}
		if ( intensity[PURPLE] < 5 )
		{
			rate = 5;
		}
		intensity[PURPLE] += rate;


		if ( state > 20 )
		{
			intensity[R1] = state;
		}

		if ( state > 40 )
		{
			intensity[R2] = state - 40;
		}

		if ( state > 60 )
		{
			intensity[R3] = state - 60;
		}

		if ( state > 80 )
		{
			intensity[R4] = state - 80;
		}

		if ( state > 100 )
		{
			intensity[Y3] = state - 100;

			intensity[R1] = 150 - state;
			intensity[R2] = 150 - state;
			intensity[R3] = 150 - state;
			intensity[R4] = 150 - state;
		}

		if ( state >= 150 )
		{
			state = 0;
			intensity[R1] = 0x0;
			intensity[R2] = 0x0;
			intensity[R3] = 0x0;
			intensity[R4] = 0x0;
			intensity[Y3] = 0x0;
			
		}
		
		_delay_ms( 20 );
	}
}

//------------------------------------------------------------------------------
void glowRandom( int cycles )
{
	for( int y=0; y<cycles; y++ )
	{
		for( unsigned char c=0 ;c<16; c++ )
		{
			intensity[c] = rand() & 0xFF;
			_delay_ms(10);
		}
	}
}

//------------------------------------------------------------------------------
void glowPick( int cycles )
{
	for( int y=0; y<cycles; y++ )
	{
		unsigned char choice = rand() & 0xF;
		intensity[choice] = 0;
		
		while( intensity[choice] < 200 )
		{
			intensity[choice] += 3;
			_delay_ms(1);
		}
		
		while( intensity[choice] > 4 )
		{
			intensity[choice] -= 3;
			_delay_ms(1);
		}

		intensity[choice] = 0;
	}
}

#define CHASE(A) intensity[(A)] = 0xFF; _delay_ms(30); intensity[(A)] = 0x0
//------------------------------------------------------------------------------
void chase( int cycles )
{
	for( int y=0; y<cycles; y++ )
	{
		CHASE(PURPLE);
		CHASE(G2);
		CHASE(Y1);
		CHASE(G3);
		CHASE(R1);
		CHASE(R2);
		CHASE(Y2);
		CHASE(G1);
		CHASE(TY1);
		CHASE(TR);
		CHASE(TG);
		CHASE(TY2);
		CHASE(G2);
		CHASE(G4);
		CHASE(R2);
		CHASE(R4);
		CHASE(Y3);
		CHASE(R3);
	}
}

//------------------------------------------------------------------------------
void randomPick( int cycles )
{
	for( int y=0; y<cycles; y++ )
	{
		unsigned char pick = rand() & 0xF;
		intensity[pick] = 0xFF;
		_delay_ms(10);
		intensity[pick] = 0x0;
	}
}

//------------------------------------------------------------------------------
int __attribute__((OS_main)) main(void)
{
	DDRC = 0xFF;
	PORTC = 0xFF;
	DDRB = 0xFF;
	PORTB = 0xFF;
	DDRD = 0xFF;
	PORTD = 0xFF;

	H_SLEEP;
	
	rnaInit();
	sei();

	TCCR0B = 1<<CS00;
	_delay_ms(100);

	srand( TCNT0 );

	TIMSK0 = 1<<TOIE0; // enable the interrupt

	for( unsigned char i=0; i<16; i++ )
	{
		intensity[i] = 0x0;
	}

	for(;;)
	{
		compute(20);
		glowPick(16);
		chase(1);
		glowRandom(20);
		chase(2);
		randomPick(128);
		chase(1);
	}
}

//------------------------------------------------------------------------------
ISR( TIMER0_OVF_vect, ISR_NOBLOCK )
{
	static unsigned char duty = 0;
	duty++;
	if ( intensity[0] > duty )
	{
		PURPLE_ON;
	}
	else
	{
		PURPLE_OFF;
	}

	if ( intensity[1] > duty )
	{
		TY1_ON;
	}
	else
	{
		TY1_OFF;
	}

	if ( intensity[2] > duty )
	{
		TR_ON;
	}
	else
	{
		TR_OFF;
	}
	
	if ( intensity[3] > duty )
	{
		TG_ON;
	}
	else
	{
		TG_OFF;
	}
	
	if ( intensity[4] > duty )
	{
		TY2_ON;
	}
	else
	{
		TY2_OFF;
	}
	
	if ( intensity[5] > duty )
	{
		G1_ON;
	}
	else
	{
		G1_OFF;
	}
	
	if ( intensity[6] > duty )
	{
		G2_ON;
	}
	else
	{
		G2_OFF;
	}
	
	if ( intensity[7] > duty )
	{
		G3_ON;
	}
	else
	{
		G3_OFF;
	}
	
	if ( intensity[8] > duty )
	{
		G4_ON;
	}
	else
	{
		G4_OFF;
	}
	
	if ( intensity[9] > duty )
	{
		R1_ON;
	}
	else
	{
		R1_OFF;
	}
	
	if ( intensity[10] > duty )
	{
		R2_ON;
	}
	else
	{
		R2_OFF;
	}
	
	if ( intensity[11] > duty )
	{
		R3_ON;
	}
	else
	{
		R3_OFF;
	}
	
	if ( intensity[12] > duty )
	{
		R4_ON;
	}
	else
	{
		R4_OFF;
	}
	
	if ( intensity[13] > duty )
	{
		Y1_ON;
	}
	else
	{
		Y1_OFF;
	}
	
	if ( intensity[14] > duty )
	{
		Y2_ON;
	}
	else
	{
		Y2_OFF;
	}
	
	if ( intensity[15] > duty )
	{
		Y3_ON;
	}
	else
	{
		Y3_OFF;
	}
}

#include "a2d.h"
/*------------------------------------------------------------------------------*
 * Copyright: (c) 2013 by Curt Hartung avr@northarc.com
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#include <avr/io.h>   
#include <avr/interrupt.h>   

//------------------------------------------------------------------------------
unsigned int a2dReadResult()
{
	while( !a2dConversionComplete() );

	unsigned int result = ADCL;
	result |= ((unsigned int)ADCH << 8);
	return result;
}

//------------------------------------------------------------------------------
unsigned int a2dReadResultNoPoll()
{
	unsigned int result = ADCL;
	result |= ((unsigned int)ADCH << 8);
	return result;
}

//------------------------------------------------------------------------------
unsigned int a2dGetTemperature()
{
	unsigned char originalChannel = ADMUX;
	
	ADMUX = 0b10100010;

	a2dStartConversion();
	a2dReadResult();
	a2dStartConversion();
	unsigned int temp = a2dReadResult();

	ADMUX = originalChannel;

	a2dStartConversion(); // and throw out next conversion after switch
	a2dReadResult();

	return temp;
}

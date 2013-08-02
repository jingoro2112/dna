#ifndef A2D_H 
#define A2D_H
/*------------------------------------------------------------------------------*/
/* Copyright: (c) 2013 by Curt Hartung
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#define a2dSetChannel( c ) (ADMUX = 0b10000000 | ((c) & 0b00000111))
#define a2dStartConversion() (ADCSRA |= (1<<ADSC))
#define a2dConversionComplete() (ADCSRA & (1<<ADSC))

#define A2D_PRESCALE_2 0b000
#define A2D_PRESCALE_4 0b010
#define A2D_PRESCALE_8 0b011
#define A2D_PRESCALE_16 0b100
#define A2D_PRESCALE_32 0b101
#define A2D_PRESCALE_64 0b110
#define A2D_PRESCALE_128 0b111
#define a2dSetPrescaler( f ) (ADCSRA = (1<<ADEN) | (f))
#define a2dEnableInterrupt() (ADCSRA |= (1<<ADIE)

unsigned int a2dReadResult();
unsigned int a2dReadResultNoPoll();
unsigned int a2dGetTemperature();


#endif 


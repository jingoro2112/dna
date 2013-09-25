#ifndef A2D_H 
#define A2D_H
/*------------------------------------------------------------------------------*
 * Copyright: (c) 2013 by Curt Hartung avr@northarc.com
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#define a2dStartConversion() (ADCSRA |= (1<<ADSC))
#define a2dWaitForConversionComplete() while(ADCSRA & (1<<ADSC));
#define a2dEnableInterrupt() (ADCSRA |= (1<<ADIE))
#define a2dReadResult() ((unsigned int)ADCL | ((unsigned int)ADCH << 8))


#endif 


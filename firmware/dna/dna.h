#ifndef DNA_H
#define DNA_H
/*------------------------------------------------------------------------------*
 * Copyright: (c) 2013 by Curt Hartung avr@northarc.com
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#include "dna_types.h"
#include "dna_defs.h" // is included in c++ projects as well so need sto remain generic

#define setLedOn() (PORTA &= 0b01111111) // A7
#define setLedOff() (PORTA |= 0b10000000) // A7
#define toggleLed() (PORTA ^= 0b10000000) // A7
#define enableLed() (DDRA |= 0b10000000) // A7

#endif

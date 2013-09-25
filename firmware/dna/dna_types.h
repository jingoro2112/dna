#ifndef DNA_TYPES_H
#define DNA_TYPES_H
/*------------------------------------------------------------------------------*
 * Copyright: (c) 2013 by Curt Hartung avr@northarc.com
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#if defined(_WIN32)
typedef unsigned short uint16;
typedef short int16;
typedef unsigned short uint;
typedef unsigned char uint8;
typedef char int8;

#elif defined(AVR)
typedef unsigned int uint16;
typedef int int16;
typedef unsigned int uint;
typedef unsigned char uint8;
typedef char int8;

#endif

#endif


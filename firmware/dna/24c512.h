#ifndef _24C512_H
#define _24C512_H
/*------------------------------------------------------------------------------*/
/* Copyright: (c) 2013 by Curt Hartung
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

void read24c512( unsigned char i2cAddress, int address, unsigned char* bytes, unsigned char len );
void write24c512( unsigned char i2cAddress, int address, unsigned char* bytes, unsigned char len );

#endif

#ifndef _24C512_H
#define _24C512_H
/*------------------------------------------------------------------------------*
 * Copyright: (c) 2013 by Curt Hartung avr@northarc.com
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

void startWrite24c512( unsigned char i2cAddress, int address );
#define streamWrite24c512( b ) do { i2cWrite(b); } while(0)
#define stop24c512() do { i2cStop(); } while(0)
void read24c512( unsigned char i2cAddress, int address, unsigned char* bytes, unsigned char len );
void write24c512( unsigned char i2cAddress, int address, unsigned char* bytes, unsigned char len );

#endif

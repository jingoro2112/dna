/* Copyright: (c) 2013 by Curt Hartung avr@northarc.com
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#include "splice.hpp"

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include <stdio.h>
#include "../firmware/dna/dna.h"
#include "../util/str.hpp"
#include "../util/architecture.hpp"

namespace Splice
{

//------------------------------------------------------------------------------
// this EXACT code signature must be found for splice to consent to
// uploading code. It is checked that the RESET vector points to this
// code block, and that it is byte-for-byte intact. The reason is
// emergency recovery in the case that pooched software is uploaded,
// this will recover the board in all but the worst-case scenario
// (where the Flash and/or bootloader has been corrupted)
const unsigned char c_bootJumper[]=
{
	0x11, 0x24, // eor	r1, r1
	0x17, 0xba, // out	0x17, r1	; 23
	0x88, 0xe0, // ldi	r24, 0x08	; 8
	0x88, 0xbb, // out	0x18, r24	; 24
	0x01, 0xb4, // in	r0, 0x21	; 33
	0x03, 0xfe, // sbrs	r0, 3
	0x06, 0xc0, // rjmp	.+12     	; 0x38a <__init+0x1a>
	0x14, 0xbe, // out	0x34, r1	; 52
	0x81, 0xb5, // in	r24, 0x21	; 33
	0x88, 0x61, // ori	r24, 0x18	; 24
	0x81, 0xbd, // out	0x21, r24	; 33
	0x11, 0xbc, // out	0x21, r1	; 33
	0x09, 0xc0, // rjmp	.+18     	; 0x39c <__init+0x2c>
	0x88, 0xec, // ldi	r24, 0xC8	; 200
	0x8a, 0x95, // dec	r24
	0xf1, 0xf7, // brne	.-4      	; 0x38c <__init+0x1c>
	0x8f, 0xef, // ldi	r24, 0xFF	; 255
	0x9f, 0xef, // ldi	r25, 0xFF	; 255
	0xb3, 0x99, // sbic	0x16, 3	; 22
	0x05, 0xc0, // rjmp	.+10     	; 0x3a2 <__init+0x32>
	0x01, 0x97, // sbiw	r24, 0x01	; 1
	0xe1, 0xf7, // brne	.-8      	; 0x394 <__init+0x24>
	0xe0, 0xea, // ldi	r30, 0xA0	; 160
	0xfc, 0xe0, // ldi	r31, 0x0C	; 12
	0x09, 0x94, // ijmp
};

//------------------------------------------------------------------------------
unsigned int checkDNAImage( const unsigned char* image, const unsigned int len, char* err /*=0*/ )
{
	if ( len >= (BOOTLOADER_ENTRY*2) )
	{
		if ( err )
		{
			sprintf( err, "code too large [0x%08X], must be below bootSplice @ 0x%08X", len, (BOOTLOADER_ENTRY*2) );
		}
		return 0;
	}

	// decompile the RESET vector rjmp instruction to find out where it is pointed to
	unsigned int offset = 2 * ( 1 + (image[0] + (((int)image[1] & 0x3F) << 8)));
	
	if ( (offset + sizeof(c_bootJumper)) >= len )
	{
		if ( err )
		{
			sprintf( err, "Invalid rjmp offset found for reset vector in image [%s]", image );
			return 0;
		}
	}
	
//	Log( "rjmp offset points to [0x%04X][0x%04X]", offset, offset / 2 );
	
	for( unsigned int i=0; i<sizeof(c_bootJumper); i++ )
	{
		if ( image[offset + i] != c_bootJumper[i] )
		{
			if ( err )
			{
				sprintf( err, "Bootjumper signature corrupt/absent [0x%02X] != [0x%02X] @ instruction #%d\n", image[offset + i], c_bootJumper[i], i/2 );
				return 0;
			}
		}
	}

	return (len * 100) / (BOOTLOADER_ENTRY * 2);
}

//------------------------------------------------------------------------------
char* stringFromId( const unsigned char id, char* buf )
{
	switch( id )
	{
		case OLED_AM88:
		{
			strcpy( buf, "ATMega88 OLED" );
			break;
		}

		case BOOTLOADER_OLED_AM88:
		{
			strcpy( buf, "BootSplice ATMega88 OLED" );
			break;
		}

		case DNA_AT84:
		{
			strcpy( buf, "ATTiny84a DNA" );
			break;
		}

		case BOOTLOADER_DNA_AT84:
		{
			strcpy( buf, "BootSplice ATTiny84a DNA" );
			break;
		}

		default:
		{
			strcpy( buf, "<undefined>" );
			break;
		}
	}

	return buf;
}

//------------------------------------------------------------------------------
bool proxyRNA( DNADEVICE device, const unsigned char header, const unsigned char target, const unsigned char command, const void* data, const int size )
{
	unsigned char packet[8000];
	packet[0] = header;
	packet[1] = target;
	packet[2] = 1 + size;
	packet[3] = command;
	if ( data && size )
	{
		memcpy( (char *)(packet + 4), (char *)data, size );
	}

	if ( !DNAUSB::sendData(device, packet, 4 + size) )
	{
		printf( "could not send RNA proxy packet\n" );
		return false;
	}

	return true;
}


}
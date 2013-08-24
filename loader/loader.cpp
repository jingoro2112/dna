/* Copyright: (c) 2013 by Curt Hartung
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#include "loader.hpp"

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include <stdio.h>
#include "../firmware/dna/dna_defs.h"
#include "../util/str.hpp"

namespace Loader
{
	
//------------------------------------------------------------------------------
// any potential code updates must have a reset vector that points to a
// function that EXACTLY MATCHES this one.
const unsigned char c_bootJumper[]=
{
	0x17, 0xba, // out     0x17, r1        ; 23
	0x88, 0xe0, // ldi     r24, 0x08       ; 8
	0x88, 0xbb, // out     0x18, r24       ; 24
	0x11, 0x24, // eor     r1, r1
	0x01, 0xb4, // in      r0, 0x21        ; 33
	0x03, 0xfe, // sbrs    r0, 3
	0x06, 0xc0, // rjmp    .+12            ; 0x36c <__init+0x1a>
	0x14, 0xbe, // out     0x34, r1        ; 52
	0x81, 0xb5, // in      r24, 0x21       ; 33
	0x88, 0x61, // ori     r24, 0x18       ; 24
	0x81, 0xbd, // out     0x21, r24       ; 33
	0x11, 0xbc, // out     0x21, r1        ; 33
	0x06, 0xc0, // rjmp    .+12            ; 0x378 <__init+0x26>
	0x88, 0xec, // ldi     r24, 0xC8       ; 200
	0x8a, 0x95, // dec     r24
	0xf1, 0xf7, // brne    .-4             ; 0x36e <__init+0x1c>
	0xb3, 0x9b, // sbis    0x16, 3 ; 22
	0x01, 0xc0, // rjmp    .+2             ; 0x378 <__init+0x26>
	0x8b, 0xce, // rjmp    .-746           ; 0x8e <__ctors_end>
	0xe0, 0xe6, // ldi     r30, 0x60       ; 112
	0xfc, 0xe0, // ldi     r31, 0x0C       ; 12
	0x09, 0x94, // ijmp
};

//------------------------------------------------------------------------------
bool checkDNAImage( const unsigned char* image, const unsigned int len, char* err /*=0*/ )
{
	if ( len >= (BOOTLOADER_ENTRY*2) )
	{
		if ( err )
		{
			sprintf( err, "code too large [0x%08X], must be below bootloader @ 0x%08X", len, (BOOTLOADER_ENTRY*2) );
		}
		return false;
	}

	// decompile the rjmp instruction to find out where it is pointed to
	unsigned int offset = 2 * ( 1 + (image[0] + (((int)image[1] & 0x3F) << 8)));
	
	if ( (offset + sizeof(c_bootJumper)) >= len )
	{
		if ( err )
		{
			sprintf( err, "Invalid rjmp offset found for reset vector in image [%s]", image );
			return false;
		}
	}
	
//	Log( "rjmp offset points to [0x%04X][0x%04X]", offset, offset / 2 );
	
	for( unsigned int i=0; i<sizeof(c_bootJumper); i++ )
	{
		if ( image[offset + i] != c_bootJumper[i] )
		{
			if ( err )
			{
				sprintf( err, "Bootjumper signature not found [0x%02X] != [0x%02X] @ instruction #%d\n", image[offset + i], c_bootJumper[i], i/2 );
				return false;
			}
		}
	}

	return true;
}

//------------------------------------------------------------------------------
DNADEVICE Loader::openDevice( char* product )
{
	DNADEVICE handle = INVALID_DNADEVICE_VALUE;

	int tries = 10;
	for( int t = 0; t < tries; t++ )
	{
//		Log( "attempting connection [%d/%d]", t+1, tries );

		// okay find our device on th USB
		char prod[256];
		handle = DNAUSB::openDevice( 0x16C0, 0x05DF, "p@northarc.com", prod );
		if ( product )
		{
			strcpy( product, prod );
		}

		if ( handle != INVALID_DNADEVICE_VALUE )
		{
			break;
		}

#ifdef _WIN32
		Sleep(2000);
#else
		sleep(2);
#endif
	}

	return handle;
}

}
/* Copyright: (c) 2013 by Curt Hartung
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include "../dnausb/dnausb.h"
#include "../firmware/dna/dna_defs.h"

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>


#include "../util/str.hpp"
#include "../util/asciidump.hpp"
#include "../util/simple_log.hpp"
#include "../util/mainargs.hpp"
#include "../util/read_hex.hpp"


SimpleLog Log;

//------------------------------------------------------------------------------
int usage()
{
	printf( "loader.exe\n"
			"\n"
			"-i     <image name, default 'application.hex'>\n"
			"-r     report only\n"
			"-oled  enumerate oled standalone board instead of dna\n"
			"-a     read a2d\n"
			"-e     eeprom read\n"
			"-v     verbose\n"
		  );
	return 0;
}

//------------------------------------------------------------------------------
// any potential code updates must have a reset vector that points to a
// function that EXACTLY MATCHES this one.
const unsigned char bootJumper[]=
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
	0xe0, 0xe7, // ldi     r30, 0x70       ; 112
	0xfc, 0xe0, // ldi     r31, 0x0C       ; 12
	0x09, 0x94, // ijmp
};

//------------------------------------------------------------------------------
int main( int argc, char *argv[] )
{
	MainArgs args( argc, argv );

	if ( !args.isSet("-v") )
	{
		DNAUSB::setLoggingCallback( 0 );
	}

	if ( args.isSet("-?") )
	{
		return usage();
	}

	char image[256] = "application.hex";
	CLinkList<ReadHex::Chunk> chunklist;
	Cstr infile;
	ReadHex::Chunk *chunk = 0;
	if ( args.isStringSet("-i", image) )
	{
		if ( !infile.fileToBuffer(image) )
		{
			Log( "Could not read [%s]", image );
			return -1;
		}

		if ( !ReadHex::parse( chunklist, infile.c_str(), infile.size() ) )
		{
			Log( "Could not parse [%s]", image );
			return -1;
		}

		if ( chunklist.count() != 1 )
		{
			Log( "multiple chunks not supported" );
			return -1;
		}
		chunk = chunklist.getFirst();

		if ( chunk->size >= (BOOTLOADER_ENTRY*2) )
		{
			Log( "code too large [0x%08X], must be below bootloader @ 0x%08X", chunk->size, (BOOTLOADER_ENTRY*2) );
			return -1;
		}

		if ( !args.isSet("-dnv") ) // double-secret 'do not valide' flag, please donot publicize. 
		{
			// decompile the rjmp instruction to find out where it is pointed to
			unsigned int offset = 2 * ( 1 + (chunk->data[0] + (((int)chunk->data[1] & 0x3F) << 8)));

			if ( (offset + sizeof(bootJumper)) >= chunk->size )
			{
				Log( "Invalid rjmp offset found for reset vector in image [%s]", image );
			}

//			Log( "rjmp offset points to [0x%04X][0x%04X]", offset, offset / 2 );

			for( int i=0; i<sizeof(bootJumper); i++ )
			{
				if ( chunk->data[offset + i] != bootJumper[i] )
				{
					Log( "Bootjumper signature not found [0x%02X] != [0x%02X] @ instruction #%d\n", chunk->data[offset + i], bootJumper[i], i/2 );
					return -1;
				}
			}
		}
	}

	Cstr vendor( "p@northarc.com" );
	if ( args.isSet("-oled") )
	{
		vendor = "oled@northarc.com";
	}

	bool trying = true;
	DNADEVICE handle = INVALID_DNADEVICE_VALUE;
	int tries = 10;
	for( int t = 0; trying && t < tries; t++ )
	{
		Log( "attempting connection [%d/%d]", t+1, tries );
		
		// okay find our device on th USB
		char product[32];
		if ( handle != INVALID_DNADEVICE_VALUE )
		{
			DNAUSB::closeDevice( handle );
		}
		handle = DNAUSB::openDevice( 0x16C0, 0x05DF, vendor, product );

		if ( handle == INVALID_DNADEVICE_VALUE )
		{
#ifdef _WIN32
			Sleep(1000);
#else
			sleep(1);
#endif
			continue;
		}

		Log( "Found: %s[0x%02X]", product, (unsigned char)product[0] );
		t = -1;
		
		unsigned char id;
		DNAUSB::getProductId( handle, &id );
		Log( "product ID[0x%02X]", id );

		if ( args.isSet("-e") )
		{
			unsigned char data[64];
			data[0] = 1;
			DNAUSB::sendData( handle, data );
#ifdef _WIN32
			Sleep( 10 );
#else
			usleep( 10000 );
#endif
			data[0] = 2;
			DNAUSB::sendData( handle, data );

			memset( data, 0xFF, 64 );
			DNAUSB::getData( handle, data );

			Log( "0x%02X 0x%02X 0x%02X 0x%02X", (unsigned int)data[0], (unsigned int)data[1], (unsigned int)data[2], (unsigned int)data[3] );
			break;
		}
		else if ( args.isSet("-a") )
		{
			float avg = 0;
			for(;;)
			{
				unsigned int data[64];
				DNAUSB::sendData( handle, (unsigned char *)data );
				DNAUSB::getData( handle, (unsigned char *)data );
				data[0] &= 0x03FF;
				avg = (avg * 30.f) + data[0];
				avg /= 31.f;
				Log( "%.2f\t%d", avg, (int)data[0] );
			}
			break;
		}
		else if (args.isSet("-r") ) // report only
		{
			break;
		}

		switch( id )
		{
			case OLED_AM88_v1_00:
			case DNA_AT84_v1_00:
			{
				Log( "Commanding bootloader entry" );
				if ( DNAUSB::sendEnterBootloader(handle) )
				{
					Log( "Sent reset command" );
#ifdef _WIN32
					Sleep(2000);
#else
					sleep(2);
#endif
				}
				DNAUSB::closeDevice( handle );
				break;
			}

			case BOOTLOADER_OLED_AM88_v1_00:
			case BOOTLOADER_DNA_AT84_v1_00:
			{
				Log( "Loading executeable" );

				if ( trying )
				{
					unsigned char checksum;
					DNAUSB::sendCode( handle, (unsigned char *)(chunk->data), chunk->size, &checksum );
					DNAUSB::sendEnterApp( handle, checksum );
				}

				trying = false;
				break;
			}

			default:
			{
				Log( "Unrecognized device [0x%02X]", (unsigned int)id );
				break;
			}
		}
	}

	DNAUSB::closeDevice( handle );
	return 0;
}

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
		  );
	return 0;
}

//------------------------------------------------------------------------------
int main( int argc, char *argv[] )
{
	MainArgs args( argc, argv );
//	DNAUSB::setLoggingCallback( 0 );

	if ( args.isSet("-?") )
	{
		return usage();
	}

	char image[256] = "application.hex";
	args.isStringSet( "-i", image );

	Cstr vendor( "p@northarc.com" );
	if ( args.isSet("-oled") )
	{
		vendor = "oled@northarc.com";
	}

	bool trying = true;
	DNADEVICE handle = INVALID_DNADEVICE_VALUE;
	for( int tries = 0; trying && tries < 10; tries++ )
	{
		// okay find our device on th USB
		char product[32];
		if ( handle != INVALID_DNADEVICE_VALUE )
		{
			DNAUSB::closeDevice( handle );
		}
		handle = DNAUSB::openDevice( 0x16C0, 0x05DF, vendor, product );

		if ( handle == INVALID_DNADEVICE_VALUE )
		{
			Log( "connect failure [%d/%d]", tries, 5 );
#ifdef _WIN32
			Sleep(1000);
#else
			sleep(1);
#endif
			continue;
		}

		Log( "Found: %s[0x%02X]", product, (unsigned char)product[0] );

		unsigned char id;
		DNAUSB::getProductId( handle, &id );
		Log( "product ID[0x%02X]", id );

		if ( args.isSet("-e") )
		{
			unsigned char data[64];
			data[0] = 1;
			DNAUSB::sendData( handle, data );
			Sleep(10);
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
					tries = 0;
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

				CLinkList<ReadHex::Chunk> chunklist;
				Cstr infile;

				if ( !infile.fileToBuffer(image) )
				{
					Log( "Could not read [%s]\n", image );
					return -1;
				}

				if ( !ReadHex::parse( chunklist, infile.c_str(), infile.size() ) )
				{
					Log( "Could not parse [%s]\n", image );
					return -1;
				}

				if ( chunklist.count() != 1 )
				{
					Log( "multiple chunks not supported\n" );
					return -1;
				}
				ReadHex::Chunk *chunk = chunklist.getFirst();

				if ( chunk->size > 0x18e0 )
				{
					Log( "code too large" );
					return -1;
				}

				// todo: check to see that the reset vector is there

				DNAUSB::sendCode( handle, (unsigned char *)(chunk->data), chunk->size );
				DNAUSB::sendEnterApp( handle, DNAUSB::getPageZeroChecksum((unsigned char *)(chunk->data)) );

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

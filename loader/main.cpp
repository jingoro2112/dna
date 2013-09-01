/* Copyright: (c) 2013 by Curt Hartung
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#include "loader.hpp"

#include "../util/read_hex.hpp"
#include "../util/str.hpp"
#include "../util/simple_log.hpp"
#include "../util/mainargs.hpp"
#include "../firmware/dna/dna_defs.h"

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

SimpleLog Log;

//------------------------------------------------------------------------------
int usage()
{
	printf( "loader.exe\n"
			"\n"
			"-i     <image name'>\n"
			"-r     report only\n"
			"-oled  enumerate oled standalone board instead of dna\n"
			"-a     read a2d\n"
			"-e     eeprom read\n"
			"-v     verbose\n"
		  );
	
	return 0;
}

//------------------------------------------------------------------------------
void loaderSleep( unsigned int microSeconds )
{
#ifdef _WIN32
	Sleep( microSeconds );
#else
	usleep( microSeconds );
#endif
}

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

		if ( !args.isSet("-dnv") ) // double-secret 'do not valide' flag, please do not publicize. 
		{
			char err[256];
			if ( !Loader::checkDNAImage(chunk->data, chunk->size, err) )
			{
				Log( "%s", err );
				return -1;
			}
		}
	}

	bool trying = true;
	char product[256];
	DNADEVICE handle;
	
	bool looping;
	do
	{
		looping = false;
		handle = Loader::openDevice( product );
		if ( handle == INVALID_DNADEVICE_VALUE )
		{
			Log( "Could not open DNA device\n" );
			return -1;
		}

		if ( args.isSet("-e") )
		{
			unsigned char data[64];
			data[0] = 1;
			DNAUSB::sendData( handle, data );

			memset( data, 0xFF, 64 );
			DNAUSB::getData( handle, data );

			Log( "0x%02X 0x%02X 0x%02X 0x%02X", (unsigned int)data[0], (unsigned int)data[1], (unsigned int)data[2], (unsigned int)data[3] );
			return 0;
		}

		if ( args.isSet("-a") )
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

			return 0;
		}

		unsigned char id;
		if ( !DNAUSB::getProductId( handle, &id ) )
		{
			Log( "failed to get product id" );
			DNAUSB::closeDevice( handle );
			handle = INVALID_DNADEVICE_VALUE;
			break;
		}

		Log( "product[%s] ID[0x%02X]", product, id );

		if (args.isSet("-r") ) // report only
		{
			return 0;
		}

		switch( id )
		{
			case OLED_AM88_v1_00:
			case DNA_AT84_v1_00:
			{
				Log( "Commanding bootloader entry" );
				if ( DNAUSB::sendEnterBootloader(handle) )
				{
					Log( "Sending reset command" );
					loaderSleep( 2000 );
					looping = true;
				}
				else
				{
					Log( "Reset command failed" );
				}
				
				DNAUSB::closeDevice( handle );
				handle = INVALID_DNADEVICE_VALUE;
				break;
			}

			case BOOTLOADER_OLED_AM88_v1_00:
			case BOOTLOADER_DNA_AT84_v1_00:
			{
				Log( "Loading executeable" );

				if ( trying )
				{
					DNAUSB::sendCode( handle, (unsigned char *)(chunk->data), chunk->size );
					DNAUSB::sendEnterApp( handle );
				}

				break;
			}

			default:
			{
				Log( "Unrecognized device [0x%02X]", (unsigned int)id );
				break;
			}
		}
		
	} while( looping );

	DNAUSB::closeDevice( handle );
	return 0;
}



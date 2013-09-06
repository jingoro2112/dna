/* Copyright: (c) 2013 by Curt Hartung
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#include <winsock2.h>
#include <windows.h>

#include "loader.hpp"

#include "../util/read_hex.hpp"
#include "../util/str.hpp"
#include "../util/simple_log.hpp"
#include "../util/mainargs.hpp"
#include "../util/architecture.hpp"
#include "../firmware/dna/dna_defs.h"

#include "../../morlock/firmware/morlock_defs.h"

#include "../firmware/dna/galloc.h"

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

SimpleLog Log;

//------------------------------------------------------------------------------
int usage()
{
	printf( "loader.exe\n"
			"\n"
			"-f, --flash <image>\n"
			"    flash firmware <image>\n"
			"    This image is checked to make sure the bootjumper code is intact\n"
			"\n"
			"-r, --report\n"
			"    Report status\n"
			"\n"
			"-v\n"
			"    verbose logging\n"
			"\n"
			"--read_eeprom <loc> <size>\n"
			"    read 'size' bytes from location 'loc'\n"
			"\n"
			"--write_eeprom <loc> <data>\n"
			"    write data to the eeprom location <loc>. <data> is in byte-hex format:\n"
			"    0102030405060708090A0B0C0D0E0F101112 ... \n"
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

unsigned int __heap_start;

//------------------------------------------------------------------------------
int main( int argc, char *argv[] )
{
	char block[1024];
	__heap_start = (unsigned int)block;
	gallocInit( 0 );

	unsigned char h1 = galloc( 10 );
	unsigned char h2 = galloc( 10 );
	unsigned char h3 = galloc( 10 );
	unsigned char h4 = galloc( 10 );

	int i;
	for( i=0; i<10; i++ )
	{
		gpointer(h1)[i] = i;
		gpointer(h2)[i] = i + 0x10;
		gpointer(h3)[i] = i + 0x20;
		gpointer(h4)[i] = i + 0x30;
	}
	
	gfree( h3 );
	gfree( h3 );

	h3 = galloc( 20 );
	for( i=0; i<20; i++ )
		gpointer(h3)[i] = i + 0x40;
	
	gfree( h1 );
	h1 = galloc( 30 );
	for( i=0; i<30; i++ )
		gpointer(h1)[i] = i + 0x50;
	
	gfree( h3 );
	gfree( h3 );
	gfree( h4 );
	h3 = galloc( 0 );
	gfree( h2 );
	gfree( h3 );
	gfree( h1 );




	

	
	MainArgs args( argc, argv );

	if ( args.isSet("-?") || args.isSet("-help") || args.isSet("--help") )
	{
		return usage();
	}

	if ( !args.isSet("-v") )
	{
		Log.setCallback( 0 );
	}

	char image[256] = "application.hex";
	CLinkList<ReadHex::Chunk> chunklist;
	Cstr infile;
	ReadHex::Chunk *chunk = 0;
	if ( args.isStringSet("-f", image) || args.isStringSet("-i", image) || args.isStringSet("--flash", image) )
	{
		if ( !infile.fileToBuffer(image) )
		{
			printf( "Could not read [%s]\n", image );
			return -1;
		}

		if ( !ReadHex::parse( chunklist, infile.c_str(), infile.size() ) )
		{
			printf( "Could not parse [%s]\n", image );
			return -1;
		}

		if ( chunklist.count() != 1 )
		{
			printf( "multiple chunks not supported\n" );
			return -1;
		}
		chunk = chunklist.getFirst();

		// double-secret 'do not valide' flag, please do not publicize,
		// this is used by people who REALLY know what they are doing
		// and want to override the boot-jumper check protection. THERE
		// IS NO GOOD REASON TO DO THIS unless you have a programmer
		// hooked up to the board and are prepared to reflash manually.
		if ( !args.isSet("-dnv") ) 
		{
			char err[256];
			if ( !Loader::checkDNAImage(chunk->data, chunk->size, err) )
			{
				printf( "%s\n", err );
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
			printf( "Could not open DNA device\n" );
			return -1;
		}

		if ( args.isSet("-t") )
		{
			if ( !DNAUSB::sendCommand( handle, ceCommandGetEEPROMConstants) )
			{
				DNAUSB::closeDevice( handle );
				printf( "command failed\n" );
				return -1;
			}

			unsigned char buffer[256];
			EEPROMConstants *consts = (EEPROMConstants *)buffer;
			if ( !DNAUSB::getData(handle, buffer) )
			{
				DNAUSB::closeDevice( handle );
				printf( "fetch failed\n" );
				return -1;
			}
			
			consts->transposeValues();

			printf( "singleSolenoid [0x%02X]\n", consts->singleSolenoid );
			printf( "fireMode [0x%02X]\n", consts->fireMode );
			printf( "ballsPerSecondX10 [0x%04X]\n", consts->ballsPerSecondX10 );
			printf( "burstCount [0x%02X]\n", consts->burstCount );
			printf( "enhancedTriggerTimeout [0x%04X]\n", consts->enhancedTriggerTimeout );
			printf( "boltHoldoff [0x%02X]\n", consts->boltHoldoff );
			printf( "accessoryRunTime [0x%02X]\n", consts->accessoryRunTime );
			printf( "dimmer [0x%02X]\n", consts->dimmer );
			printf( "ABSTimeout [0x%04X]\n", consts->ABSTimeout );
			printf( "ABSAddition [0x%02X]\n", consts->ABSAddition );
			printf( "rebounce [0x%02X]\n", consts->rebounce );
			printf( "debounce [0x%02X]\n", consts->debounce );
			printf( "dwell1 [0x%02X]\n", consts->dwell1 );
			printf( "dwell2Holdoff [0x%02X]\n", consts->dwell2Holdoff );
			printf( "dwell2 [0x%02X]\n", consts->dwell2 );
			printf( "maxDwell2 [0x%02X]\n", consts->maxDwell2 );
			printf( "eyeHoldoff [0x%02X]\n", consts->eyeHoldoff );
			printf( "eyeHighBlocked [0x%02X]\n", consts->eyeHighBlocked );
			printf( "eyeEnabled [0x%02X]\n", consts->eyeEnabled );
			printf( "locked [0x%02X]\n", consts->locked );
			printf( "rampEnableCount [0x%02X]\n", consts->rampEnableCount );
			printf( "rampClimb [0x%02X]\n", consts->rampClimb );
			printf( "rampTopMode [0x%02X]\n", consts->rampTopMode );
			printf( "rampTimeout [0x%04X]\n", consts->rampTimeout );




			consts->singleSolenoid = 0x55;
			consts->fireMode = 0x55;
			consts->ballsPerSecondX10 = 0x55;
			consts->burstCount = 0x55;
			consts->enhancedTriggerTimeout = 0x55;
			consts->boltHoldoff = 0x55;
			consts->accessoryRunTime = 0x55;
			consts->dimmer = 0x55;
			consts->ABSTimeout = 0x55;
			consts->ABSAddition = 0x55;
			consts->rebounce = 0x55;
			consts->debounce = 0x55;
			consts->dwell1 = 0x55;
			consts->dwell2Holdoff = 0x55;
			consts->dwell2 = 0x55;
			consts->maxDwell2 = 0x55;
			consts->eyeHoldoff = 0x55;
			consts->eyeHighBlocked = 0x55;
			consts->eyeEnabled = 0x55;
			consts->locked = 0x55;
			consts->rampEnableCount = 0x55;
			consts->rampClimb = 0x55;
			consts->rampTopMode = 0x55;
			consts->rampTimeout = 0x55;




			
			consts->transposeValues();
			buffer[0] = ceCommandSetEEPROMConstants;
			memcpy( buffer + 1, consts, sizeof(EEPROMConstants) );
			if ( !DNAUSB::sendData(handle, buffer, sizeof(EEPROMConstants) + 1 ) )
			{
				DNAUSB::closeDevice( handle );
				printf( "put failed\n" );
				return -1;
			}



			if ( !DNAUSB::sendCommand( handle, ceCommandGetEEPROMConstants) )
			{
				DNAUSB::closeDevice( handle );
				printf( "command failed\n" );
				return -1;
			}


			if ( !DNAUSB::getData(handle, buffer) )
			{
				DNAUSB::closeDevice( handle );
				printf( "fetch failed\n" );
				return -1;
			}

			consts->transposeValues();

			printf( "singleSolenoid [0x%02X]\n", consts->singleSolenoid );
			printf( "fireMode [0x%02X]\n", consts->fireMode );
			printf( "ballsPerSecondX10 [0x%04X]\n", consts->ballsPerSecondX10 );
			printf( "burstCount [0x%02X]\n", consts->burstCount );
			printf( "enhancedTriggerTimeout [0x%04X]\n", consts->enhancedTriggerTimeout );
			printf( "boltHoldoff [0x%02X]\n", consts->boltHoldoff );
			printf( "accessoryRunTime [0x%02X]\n", consts->accessoryRunTime );
			printf( "dimmer [0x%02X]\n", consts->dimmer );
			printf( "ABSTimeout [0x%04X]\n", consts->ABSTimeout );
			printf( "ABSAddition [0x%02X]\n", consts->ABSAddition );
			printf( "rebounce [0x%02X]\n", consts->rebounce );
			printf( "debounce [0x%02X]\n", consts->debounce );
			printf( "dwell1 [0x%02X]\n", consts->dwell1 );
			printf( "dwell2Holdoff [0x%02X]\n", consts->dwell2Holdoff );
			printf( "dwell2 [0x%02X]\n", consts->dwell2 );
			printf( "maxDwell2 [0x%02X]\n", consts->maxDwell2 );
			printf( "eyeHoldoff [0x%02X]\n", consts->eyeHoldoff );
			printf( "eyeHighBlocked [0x%02X]\n", consts->eyeHighBlocked );
			printf( "eyeEnabled [0x%02X]\n", consts->eyeEnabled );
			printf( "locked [0x%02X]\n", consts->locked );
			printf( "rampEnableCount [0x%02X]\n", consts->rampEnableCount );
			printf( "rampClimb [0x%02X]\n", consts->rampClimb );
			printf( "rampTopMode [0x%02X]\n", consts->rampTopMode );
			printf( "rampTimeout [0x%04X]\n", consts->rampTimeout );

			
			

			DNAUSB::closeDevice( handle );
			return 0;
		}
		
		unsigned char id;
		if ( !DNAUSB::getProductId( handle, &id ) )
		{
			printf( "failed to get product id\n" );
			DNAUSB::closeDevice( handle );
			handle = INVALID_DNADEVICE_VALUE;
			break;
		}

		char buf[256];
		printf( "product[%s] [0x%02X]:%s\n", product, id, Loader::stringFromId(id, buf) );

		if (args.isSet("-r") || args.isSet("--report") ) // report only
		{
			return 0;
		}

		switch( id )
		{
			case OLED_AM88_v1_00:
			case DNA_AT84_v1_00:
			{
				printf( "Commanding bootloader entry\n" );
				if ( DNAUSB::sendEnterBootloader(handle) )
				{
					loaderSleep( 2000 );
					looping = true;
				}
				else
				{
					printf( "Reset command failed\n" );
				}
				
				DNAUSB::closeDevice( handle );
				handle = INVALID_DNADEVICE_VALUE;
				break;
			}

			case BOOTLOADER_OLED_AM88_v1_00:
			case BOOTLOADER_DNA_AT84_v1_00:
			{
				printf( "Loading executable\n" );

				if ( trying )
				{
					DNAUSB::sendCode( handle, (unsigned char *)(chunk->data), chunk->size );
				}

				break;
			}

			default:
			{
				printf( "Unrecognized device [0x%02X]\n", (unsigned int)id );
				break;
			}
		}
		
	} while( looping );

	DNAUSB::closeDevice( handle );
	return 0;
}



/*------------------------------------------------------------------------------*
 * Copyright: (c) 2013 by Curt Hartung avr@northarc.com
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#include "splice.hpp"

#include "../util/read_hex.hpp"
#include "../util/str.hpp"
#include "../util/simple_log.hpp"
#include "../util/mainargs.hpp"
#include "../util/architecture.hpp"
#include "../firmware/dna/dna.h"
#include "../firmware/dna/rna_packet.h"
#include "../firmware/morlock/morlock_defs.h"
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

SimpleLog Log;

//------------------------------------------------------------------------------
int usage()
{
	printf( "splice version: %d.%02d\n"
			"Usage: splice <options>\n"
			"\n"
			"-c, --command <byte1[,byte2,byte3,...]>\n"
			"    send up to 6 command bytes to the DNA board. They must be comma-delimted\n"
			"    but can be in any format understood by atoi\n"
			"    The first of these bytes will be interpreted as the command header, the\n"
			"    remaining 5 will be passed as data\n\n"
			
			"-e, --load_eeprom <binfile>\n"
			"    Load the OLED EEPROM with the binary data found in binfile\n\n"

			"-f, --flash <image>\n"
			"    flash firmware <image>\n"
			"    This image is checked to make sure the bootjumper code is intact\n\n"

			"--format\n"
			"    Kick the DNA into it's bootloader and exit\n\n"
			
			"-m, --monitor\n"
			"    Monitor target for debug messages\n\n"

			"-o, --oled <image>\n"
			"    send OLED boot image through morlock board\n\n"

			"-b, --button <image>\n"
			"    send Button board boot image through morlock board\n\n"

			"-r, --report\n"
			"    Report status\n\n"

			"-v\n"
			"    verbose logging\n\n"

			"--version, --help, -help, -?\n"
			"    print this message and exit\n\n"
			,c_majorVersion,
			c_minorVersion
		  );
	
	return 0;
}

//------------------------------------------------------------------------------
int main( int argc, char *argv[] )
{
	MainArgs args( argc, argv );

	if ( (argc == 1) || args.isSet("-?") || args.isSet("-help") || args.isSet("--help") || args.isSet("--version") )
	{
		return usage();
	}

	if ( !args.isSet("-v") )
	{
		Log.setCallback( 0 );
	}

	char product[256];
	DNADEVICE device;
	
	device = DNAUSB::openDevice( 0x16C0, 0x05DF, "p@northarc.com", product );
	if ( device == INVALID_DNADEVICE_VALUE )
	{
		printf( "Could not open DNA device\n" );
		return -1;
	}

	if ( args.isSet("-t") )
	{
		unsigned char buffer[256];
		EEPROMConstants *consts = (EEPROMConstants *)buffer;

		for(;;)
		{
			Arch::sleep(1000);

		if ( !DNAUSB::sendCommand(device, ceCommandGetEEPROMConstants) )
		{
			DNAUSB::closeDevice( device );
			printf( "command failed\n" );
			return -1;
		}

		unsigned char len;
		if ( !DNAUSB::getData(device, buffer, &len) )
		{
			DNAUSB::closeDevice( device );
			printf( "fetch failed\n" );
			return -1;
		}

		Arch::asciiDump( consts, sizeof(*consts) );

		printf( "len[%d]\n", (int)len );
		printf( "singleSolenoid[%08X][%d]\n", (unsigned int)consts->singleSolenoid, (unsigned int)consts->singleSolenoid );
		printf( "fireMode[%08X][%d]\n", (unsigned int)consts->fireMode, (unsigned int)consts->fireMode );
		printf( "ballsPerSecondX10[%08X][%d]\n", (unsigned int)consts->ballsPerSecondX10, (unsigned int)consts->ballsPerSecondX10 );
		printf( "burstCount[%08X][%d]\n", (unsigned int)consts->burstCount, (unsigned int)consts->burstCount );
		printf( "enhancedTriggerTimeout[%08X][%d]\n", (unsigned int)consts->enhancedTriggerTimeout, (unsigned int)consts->enhancedTriggerTimeout );
		printf( "boltHoldoff[%08X][%d]\n", (unsigned int)consts->boltHoldoff, (unsigned int)consts->boltHoldoff );
		printf( "accessoryRunTime[%08X][%d]\n", (unsigned int)consts->accessoryRunTime, (unsigned int)consts->accessoryRunTime );
		printf( "dimmer[%08X][%d]\n", (unsigned int)consts->dimmer, (unsigned int)consts->dimmer );
		printf( "ABSTimeout[%08X][%d]\n", (unsigned int)consts->ABSTimeout, (unsigned int)consts->ABSTimeout );
		printf( "ABSAddition[%08X][%d]\n", (unsigned int)consts->ABSAddition, (unsigned int)consts->ABSAddition );
		printf( "rebounce[%08X][%d]\n", (unsigned int)consts->rebounce, (unsigned int)consts->rebounce );
		printf( "debounce[%08X][%d]\n", (unsigned int)consts->debounce, (unsigned int)consts->debounce );
		printf( "dwell1[%08X][%d]\n", (unsigned int)consts->dwell1, (unsigned int)consts->dwell1 );
		printf( "dwell2Holdoff[%08X][%d]\n", (unsigned int)consts->dwell2Holdoff, (unsigned int)consts->dwell2Holdoff );
		printf( "dwell2[%08X][%d]\n", (unsigned int)consts->dwell2, (unsigned int)consts->dwell2 );
		printf( "maxDwell2[%08X][%d]\n", (unsigned int)consts->maxDwell2, (unsigned int)consts->maxDwell2 );
		printf( "eyeEnabled[%08X][%d]\n", (unsigned int)consts->eyeEnabled, (unsigned int)consts->eyeEnabled );
		printf( "eyeHoldoff[%08X][%d]\n", (unsigned int)consts->eyeHoldoff, (unsigned int)consts->eyeHoldoff );
		printf( "eyeHighBlocked[%08X][%d]\n", (unsigned int)consts->eyeHighBlocked, (unsigned int)consts->eyeHighBlocked );
		printf( "eyeDetectLevel[%08X][%d]\n", (unsigned int)consts->eyeDetectLevel, (unsigned int)consts->eyeDetectLevel );
		printf( "eyeDetectHoldoff[%08X][%d]\n", (unsigned int)consts->eyeDetectHoldoff, (unsigned int)consts->eyeDetectHoldoff );
		printf( "locked[%08X][%d]\n", (unsigned int)consts->locked, (unsigned int)consts->locked );
		printf( "rampEnableCount[%08X][%d]\n", (unsigned int)consts->rampEnableCount, (unsigned int)consts->rampEnableCount );
		printf( "rampClimb[%08X][%d]\n", (unsigned int)consts->rampClimb, (unsigned int)consts->rampClimb );
		printf( "rampTopMode[%08X][%d]\n", (unsigned int)consts->rampTopMode, (unsigned int)consts->rampTopMode );
		printf( "rampTimeout[%08X][%d]\n", (unsigned int)consts->rampTimeout, (unsigned int)consts->rampTimeout );
		printf( "shortCyclePreventionInterval[%08X][%d]\n", (unsigned int)consts->shortCyclePreventionInterval, (unsigned int)consts->shortCyclePreventionInterval );
		printf( "eyeLevel[%08X][%d]\n", (unsigned int)consts->eyeLevel, (unsigned int)consts->eyeLevel );

		printf( "---\n" );
		printf( "eye[0x%02X] %s\n", (int)consts->eyeLevel, (consts->eyeLevel < consts->eyeDetectLevel) ? "blocked":"clear" );
		printf( "version[%d]\n", (int)consts->version );

		}
		DNAUSB::closeDevice( device );
		return 0;
	}

	unsigned char id;
	unsigned char version;
	if ( !DNAUSB::getProductId( device, &id, &version ) )
	{
		printf( "failed to get product id\n" );
		DNAUSB::closeDevice( device );
		return 0;
	}
	char buf[256];
	printf( "product[%s] [0x%02X:%s] version[%d]\n", product, id, Splice::stringFromId(id, buf), version );
	
	if (args.isSet("-r") || args.isSet("--report") ) // report only
	{
		return 0;
	}

	if ( args.isStringSet("-c", buf) || args.isStringSet("--command", buf) )
	{
		unsigned char command[6] = { 0, 0, 0, 0, 0 };

		unsigned int pos = 0;
		Cstr token;
		for( unsigned int i=0; i<6; i++ )
		{
			token.clear();
			for( ; buf[pos] && buf[pos] != ','; pos++ )
			{
				token += buf[pos];
			}
			pos++;
			command[i] = (unsigned char)strtol( token, 0, 0 );
		}

		if ( !DNAUSB::sendCommand(device, command[0], command + 1) )
		{
			DNAUSB::closeDevice( device );
			printf( "sending command failed\n" );
			return -1;
		}

		printf( "Send command 0x%02X : 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\n",
				command[0], 
				command[1], 
				command[2], 
				command[3], 
				command[4], 
				command[5] );

		return 0;
	}

	if ( args.isSet("--format") )
	{
		if ( !DNAUSB::sendEnterBootloader(device) )
		{
			Log( "failed to send bootloader entery request" );
			DNAUSB::closeDevice( device );
			return -1;
		}

		printf( "sent bootloader entry request\n" );
		return 0;
	}

	if ( args.isSet("-m") || args.isSet("--monitor") )
	{
		Arch::sleep( 100 );

		unsigned char buffer[256];
		for(;;)
		{
			if ( !DNAUSB::getPrintMessage(device, (char*)buffer) )
			{
				DNAUSB::closeDevice( device );
				printf( "get print message failed\n" );
				return -1;
			}

			if ( buffer[0] )
			{
				printf( "> %s\n", buffer );
			}
			
			Arch::sleep(50);
		}
	}
	
	char image[256] = "application.hex";

	if ( args.isStringSet("-e", image) || args.isStringSet("--load_eeprom", image) )
	{
		Cstr bin;
		if ( !bin.fileToBuffer(image) )
		{
			printf( "could not read [%s] for EEPROM push\n", image );
			return usage();
		}

		unsigned int position = 0;
		PacketEEPROMLoad load;
		load.offset = 0;
		while( position < bin.size() )
		{
			for( unsigned int i=0; position < bin.size() && i<sizeof(load.data); position++, i++ )
			{
				load.data[i] = bin[position];
			}

			Splice::proxyRNA( device, ceCommandRNASend, RNADeviceOLED, RNATypeEEPROMLoad, &load, sizeof(PacketEEPROMLoad) );
			load.offset += 128;
			Arch::sleep( 1000 ); // give hardware a break to finish the write cycle
		}

		return 0;
	}
	
	CLinkList<ReadHex::Chunk> chunklist;
	Cstr infile;
	ReadHex::Chunk *chunk = 0;
	unsigned int percent;

	if ( args.isStringSet("-o", image) || args.isStringSet("--oled", image) )
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

		Splice::proxyRNA( device, ceCommandRNASend, RNADeviceOLED, RNATypeEnterBootloader );
		Arch::sleep( 100 );
		Splice::proxyRNA( device, ceCommandRNASend, RNADeviceOLED, RNATypeEnterBootloader );
		Arch::sleep( 100 );

		unsigned short address = 64;

		PacketEnterApp enter;
		enter.lastAddress = chunk->size;
		enter.checksum = 0;

		while( address < chunk->size )
		{
			PacketCodePage code;
			code.page = address;
			for( int i=0; i<64; i += 2 ) // the code
			{
				if ( address < chunk->size )
				{
					code.code[i/2] = chunk->data[address] | (chunk->data[address+1] << 8);
					enter.checksum += code.code[i/2];
				}
				else
				{
					code.code[i/2] = 0x0;
				}

				address += 2;
			}
			Splice::proxyRNA( device, ceCommandRNASend, RNADeviceOLED, RNATypeCodePage, &code, sizeof(PacketCodePage) );
			
			Arch::sleep( 75 );

			if ( address == 64 )
			{
				Log( "[%d/%d]", chunk->size / 64, chunk->size / 64 );
				Splice::proxyRNA( device,
								  ceCommandRNASend,
								  RNADeviceOLED,
								  RNATypeEnterApp,
								  &enter, sizeof(PacketEnterApp) );
				break;
			}

			Log( "[%d/%d]", (address / 64) - 1, chunk->size / 64 );

			if ( address >= chunk->size )
			{
				address = 0;
			}
		}

		DNAUSB::closeDevice( device );
	}

	if ( args.isStringSet("-b", image) || args.isStringSet("--button", image) )
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

		Splice::proxyRNA( device, ceCommandRNASend, RNADeviceBUTTON, RNATypeEnterBootloader );
		Arch::sleep( 100 );
		Splice::proxyRNA( device, ceCommandRNASend, RNADeviceBUTTON, RNATypeEnterBootloader );
		Arch::sleep( 100 );

		unsigned short address = 32;

		PacketEnterApp enter;
		enter.lastAddress = chunk->size;
		enter.checksum = 0;

		while( address < chunk->size )
		{
			PacketButtonCodePage code;
			code.page = address;
			for( int i=0; i<32; i += 2 ) // the code
			{
				if ( address < chunk->size )
				{
					code.code[i/2] = chunk->data[address] | (chunk->data[address+1] << 8);
					enter.checksum += code.code[i/2];
				}
				else
				{
					code.code[i/2] = 0x0;
				}

				address += 2;
			}
			Splice::proxyRNA( device,
							  ceCommandRNASend,
							  RNADeviceBUTTON,
							  RNATypeCodePage,
							  &code, sizeof(PacketButtonCodePage) );

			Arch::sleep( 75 );

			if ( address == 32 )
			{
				Log( "[%d/%d]", chunk->size / 32, chunk->size / 32 );
				Splice::proxyRNA( device,
								  ceCommandRNASend,
								  RNADeviceBUTTON,
								  RNATypeEnterApp,
								  &enter, sizeof(PacketEnterApp) );
				break;
			}

			Log( "[%d/%d]", (address / 32) - 1, chunk->size / 32 );

			if ( address >= chunk->size )
			{
				address = 0;
			}
		}

		DNAUSB::closeDevice( device );
	}

	
	if ( args.isStringSet("-f", image) || args.isStringSet("-i", image) || args.isStringSet("--flash", image) )
	{
		DNAUSB::closeDevice( device );

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

		// double-secret 'Do Not Validate' flag, please do not publicize,
		// this is used by people who REALLY know what they are doing
		// and want to override the boot-jumper check protection. THERE
		// IS NO GOOD REASON TO DO THIS UNLESS YOU HAVE A PROGRAMMER, A
		// SOLDERING IRON AND TOO MUCH TIME ON YOUR HANDS
		if ( !args.isSet("-dnv") ) 
		{
			char err[256];
			percent = Splice::checkDNAImage( chunk->data, chunk->size, err );
			if ( !percent )
			{
				printf( "%s\n", err );
				return -1;
			}
		}

		DNAUSB::sendCode( 0x16C0, 0x05DF, "p@northarc.com", (unsigned char *)(chunk->data), chunk->size, 0 );
	}

	return 0;
}

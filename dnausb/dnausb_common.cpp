/*------------------------------------------------------------------------------*/
/* Copyright: (c) 2013 by Curt Hartung
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#include "dnausb.h"

#include "../util/simple_log.hpp"
#include "../util/architecture.hpp"
#include "../util/str.hpp"

extern SimpleLog Log;

#include "../firmware/dna/dna_defs.h"

//------------------------------------------------------------------------------
bool DNAUSB::getProductId( DNADEVICE device, unsigned char *id )
{
	unsigned char packet[8] = { Report_DNA };

	if ( !HID_GetFeature(device, packet, 8) )
	{
		Log( "failed to get status" );
		return false;
	}

	*id = packet[1];
	return true;
}

//------------------------------------------------------------------------------
static bool sendSystemCommand( DNADEVICE device, const unsigned char cmd, const unsigned char param =0 )
{
	unsigned char comm[8] = { Report_DNA };
	comm[1] = cmd;
	comm[2] = param;
	
	if ( !DNAUSB::HID_SetFeature(device, comm, 8) )
	{
		Log( "failed to send command [%d]", (int)cmd );
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------
bool DNAUSB::sendEnterBootloader( DNADEVICE device )
{
	return sendSystemCommand( device, USBCommandEnterBootloader );
}

//------------------------------------------------------------------------------
bool DNAUSB::sendCode( DNADEVICE device, const unsigned char* code, const unsigned int size )
{
	unsigned char codePage[8] = { Report_DNA };

	// todo- some fancy down-count so this is done in a single pass
	// rather than up from page 1 then second pass for page zero. like
	// I have the time to be fancy?

	// upload all the code starting from page 1, so the Interrupt
	// Vector Table is loaded last
	unsigned short address = 64;
	unsigned int pos = 64;
	while( pos < size )
	{
		for( int i=0; i<32; i += 2 )
		{
			if ( pos < size )
			{
				codePage[1] = i*2;
				codePage[2] = code[pos++];
				codePage[3] = code[pos++];
				codePage[4] = code[pos++];
				codePage[5] = code[pos++];
			}
			else
			{
				pos += 4; // so the printf reads properly
			}

			if ( !HID_SetFeature(device, codePage, 8) )
			{
				Log( "failed to sendCode <1>" );
				return false;
			}
		}

		Log( "page[%d/%d]", (pos/64) - 1, size/64 );

		codePage[1] = BootloaderCommandCommitPage;
		*(unsigned short *)(codePage + 2) = address;
		address += 64;

		if ( !HID_SetFeature(device, codePage, 8) )
		{
			Log( "failed to send page commit <1>" );
			return false;
		}

		Arch::sleep( 10 ); // nmust give the chip time to commit the page or bad things happen
	}

	// now load page zero
	pos = 0;
	for( int i=0; i<32; i += 2 )
	{
		if ( pos < size )
		{
			codePage[1] = i*2;
			codePage[2] = code[pos++];
			codePage[3] = code[pos++];
			codePage[4] = code[pos++];
			codePage[5] = code[pos++];
		}

		if ( !HID_SetFeature(device, codePage, 8) )
		{
			Log( "failed to sendCode <2>" );
			return false;
		}
	}

	// comitting this page will also cause the bootloader to jump to
	// the new code, if the checksum passes
	codePage[1] = BootloaderCommandCommitPage;
	codePage[2] = 0;
	codePage[3] = 0;
	*(unsigned short *)(codePage + 4) = size;
	codePage[6] = 0;
	for( unsigned int j=0; j<size; j++ )
	{
		codePage[6] += code[j];
	}

	if ( !HID_SetFeature(device, codePage, 8) )
	{
		Log( "failed to send page commit <2>" );
		return false;
	}

	printf( "comitted zero page, app should be running\n" );

	Arch::sleep( 10 ); // nmust give the chip time to commit the page or bad things happen

	return true;
}

//------------------------------------------------------------------------------
bool DNAUSB::sendData( DNADEVICE device, const unsigned char* data, const unsigned char size )
{
	unsigned char packet[131] = { Report_DNA_Data };
	packet[1] = size;
	memcpy( packet + 2, data, size );

	if ( !HID_SetFeature(device, packet, 131) )
	{
		Log( "failed to send data" );
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------
bool DNAUSB::sendCommand( DNADEVICE device, const unsigned char command, const unsigned char data[5] )
{
	unsigned char packet[8] = { Report_DNA };
	packet[1] = USBCommandUser;
	packet[2] = command;
	if ( data )
	{
		memcpy( packet + 3, data, 5 );
	}

	if ( !HID_SetFeature(device, packet, 8) )
	{
		Log( "failed to send command" );
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------
bool DNAUSB::getData( DNADEVICE device, unsigned char* data, unsigned char* size /*=0*/ )
{
	unsigned char message[131] = { Report_DNA_Data };
	for(;;)
	{
		if ( !DNAUSB::HID_GetFeature(device, message, 131) )
		{
			Log( "failed to poll for available data" );
			return false;
		}

		if ( message[1] )
		{
			break;
		}

		// zero size, poll until the data is available I guess
		Arch::sleep( 100 );
	}

	if ( size )
	{
		*size = message[1];
	}
	
	memcpy( data, message + 2, message[1] );
	return true;
}


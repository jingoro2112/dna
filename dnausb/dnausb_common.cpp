/* Copyright: (c) 2013 by Curt Hartung avr@northarc.com
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#include "dnausb.h"

#include "../util/simple_log.hpp"
#include "../util/architecture.hpp"
#include "../util/str.hpp"
#include "../firmware/dna/dna_defs.h"

extern SimpleLog Log;


//------------------------------------------------------------------------------
bool DNAUSB::getProductId( DNADEVICE device, unsigned char *id, unsigned char *version )
{
	unsigned char packet[REPORT_DNA_SIZE + 1] = { REPORT_DNA };

	if ( !HID_GetFeature(device, packet, REPORT_DNA_SIZE + 1) )
	{
		Log( "failed to get status" );
		return false;
	}

	*id = packet[1];
	if ( version )
	{
		*version = packet[2];
	}
	return true;
}

//------------------------------------------------------------------------------
static bool sendSystemCommand( DNADEVICE device, const unsigned char cmd, const unsigned char param =0 )
{
	unsigned char comm[REPORT_DNA_SIZE + 1] = { REPORT_DNA };
	comm[1] = cmd;
	comm[2] = param;
	
	if ( !DNAUSB::HID_SetFeature(device, comm, REPORT_DNA_SIZE + 1) )
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
bool DNAUSB::sendCode( const int vid, const int pid, const char* vendor,
					   const unsigned char* code,
					   const unsigned int size,
					   void (*status)(const unsigned int percent) /*=0*/ )
{
	bool confirm = false;
	DNADEVICE device;

	if ( status )
	{
		status( 25 );
	}

	int tries = 0;
	for(;;)
	{
		Arch::sleep( 100 );

		char product[256];
		for( ; tries < 10; tries++ )
		{
			if ( (device = DNAUSB::openDevice(vid, pid, vendor, product)) != INVALID_DNADEVICE_VALUE )
			{
				break;
			}

			Arch::sleep( 2000 );
		}

		if ( tries >= 10 )
		{
			Log( "Could not open device" );
			return false;
		}

		unsigned char id = 0;
		if ( !DNAUSB::getProductId(device, &id) )
		{
			Log( "failed to get product id" );
			DNAUSB::closeDevice( device );
		}

		switch( id )
		{
			case BOOTLOADER_OLED_AM88:
			case BOOTLOADER_DNA_AT84:
			{
				unsigned char codePage[REPORT_DNA_SIZE + 1] = { REPORT_DNA };

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

						if ( !DNAUSB::HID_SetFeature(device, codePage, REPORT_DNA_SIZE + 1) )
						{
							Log( "failed to sendCode <1>" );
							DNAUSB::closeDevice( device );
							return false;
						}
					}

					if ( status )
					{
						status( (((pos/64) - 1) * 100) / ((size/64)+1) );
					}
					Log( "page[%d/%d]", (pos/64) - 1, size/64 );

					codePage[1] = BootloaderCommandCommitPage;
					*(unsigned short *)(codePage + 2) = address;
					address += 64;

					if ( !DNAUSB::HID_SetFeature(device, codePage, REPORT_DNA_SIZE + 1) )
					{
						Log( "failed to send page commit <1>" );
						DNAUSB::closeDevice( device );
						return false;
					}

					Arch::sleep( 15 ); // nmust give the chip time to commit the page or bad things happen
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

					if ( !DNAUSB::HID_SetFeature(device, codePage, REPORT_DNA_SIZE + 1) )
					{
						Log( "failed to sendCode <2>" );
						DNAUSB::closeDevice( device );
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

				if ( !DNAUSB::HID_SetFeature(device, codePage, REPORT_DNA_SIZE + 1) )
				{
					Log( "failed to send page commit <2>" );
					DNAUSB::closeDevice( device );
					return false;
				}

				// "comitted zero page, app should be running
				
				Arch::sleep( 100 ); // must give the I/O layer time to commit/flush
				
				confirm = true;
				DNAUSB::closeDevice( device );
				device = INVALID_DNADEVICE_VALUE;
				
				break;
			}

			default:
			{
				if ( confirm )
				{
					if ( status )
					{
						status( 100 );
					}

					printf( "Flashed to: product[%s] id[0x%02X]\n", product, id );

					DNAUSB::closeDevice( device );
					return true;
				}

				Log( "found something other than the bootloader, trying to command it to enter the bootloader" );

				if ( status )
				{
					status( 25 );
				}
				
				if ( !DNAUSB::sendEnterBootloader(device) )
				{
					Log( "failed to send bootloader enter" );
					DNAUSB::closeDevice( device );
					return false;
				}

				DNAUSB::closeDevice( device );

				Arch::sleep( 1000 );
				
				device = INVALID_DNADEVICE_VALUE;
				break;
			}
		}
	}
}

//------------------------------------------------------------------------------
bool DNAUSB::sendData( DNADEVICE device, const unsigned char* data, const unsigned int size )
{
	unsigned char packet[8000] = { REPORT_DNA_DATA };

	unsigned int offset = 0;
	int sz = (int)size;
	do
	{
		packet[1] = (sz > MAX_USER_DATA_REPORT_SIZE) ? (MAX_USER_DATA_REPORT_SIZE) : sz;
		memcpy( packet + 2, data + offset, packet[1]);

//		Arch::asciiDump( packet, REPORT_DNA_DATA_SIZE + 1 );

		if ( !HID_SetFeature(device, packet, REPORT_DNA_DATA_SIZE + 1) )
		{
			Log( "failed to send data" );
			return false;
		}

		offset += MAX_USER_DATA_REPORT_SIZE;
		sz -= MAX_USER_DATA_REPORT_SIZE;

		Arch::sleep( 15 );
		
	} while( sz > 0 );

	return true;
}

//------------------------------------------------------------------------------
bool DNAUSB::sendCommand( DNADEVICE device, const unsigned char command, const unsigned char data[5] )
{
	unsigned char packet[REPORT_DNA_SIZE + 1] = { REPORT_DNA };
	packet[1] = USBCommandUser;
	packet[2] = command;
	if ( data )
	{
		memcpy( packet + 3, data, 5 );
	}

	if ( !HID_SetFeature(device, packet, REPORT_DNA_SIZE + 1) )
	{
		Log( "failed to send command" );
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------
bool DNAUSB::getData( DNADEVICE device, unsigned char* data, unsigned char* size )
{
	unsigned char message[REPORT_DNA_DATA_SIZE + 1] = { REPORT_DNA_DATA };
	memset( message + 1, 0xFE, REPORT_DNA_DATA_SIZE );
	
	if ( !DNAUSB::HID_GetFeature(device, message, REPORT_DNA_DATA_SIZE + 1) )
	{
		printf( "failed to poll for available data" );
		return false;
	}

	if ( (*size = message[1]) )
	{
		memcpy( data, message + 2, *size );
	}
	
	return true;
}


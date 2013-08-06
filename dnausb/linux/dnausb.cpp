#include "../dnausb.h"

#include "../../firmware/dna/dna_defs.h"

#include "../../util/simple_log.hpp"
#include "../../util/asciidump.hpp"

static SimpleLog Log;

//------------------------------------------------------------------------------
hid_device* DNAUSB::openDevice( int vid, int pid, const char* vendor, char* product )
{
	hid_init();

	hid_device* device = hid_open( vid, pid, NULL);
	if ( !device )
	{
		Log( "could not open device" );
		return 0;
	}

	char buf[64];
	Cstr temp;
	if ( hid_get_manufacturer_string( device, buf, 64) )
	{
		Log( "could not fetch manufacturer string" );
		hid_close( device );
		return 0;
	}

	if ( strcmp( vendor, buf) )
	{
		Log( "no match [%s]", vendor );
		hid_close( device );
		return 0;
	}

	if ( hid_get_product_string( device, product, 64) )
	{
		Log( "could not fetch product string" );
		hid_close( device );
		return 0;
	}


	return device;
}

//------------------------------------------------------------------------------
void DNAUSB::closeDevice( hid_device* device )
{
	hid_close( device );
}

//------------------------------------------------------------------------------
bool DNAUSB::getProductId( hid_device* device, unsigned char *id )
{
	unsigned char comm[66] = { Report_Command };
	if ( hid_get_feature_report(device, comm, 66) == -1 )
	{
		Log( "failed to get productId" );
		return false;
	}

	*id = comm[1] & DNA_PRODUCT_MASK;

	return true;
}

//------------------------------------------------------------------------------
static bool sendCommand( hid_device* device, char cmd, const unsigned char param =0 )
{
	unsigned char comm[66] = { Report_Command, cmd, param };
	if ( hid_send_feature_report(device, comm, 66) == -1 )
	{
		Log( "failed to send command" );
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------
bool DNAUSB::sendEnterApp( hid_device* device, unsigned char checksum )
{
	return sendCommand( device, USBCommandEnterApp, checksum );
}

//------------------------------------------------------------------------------
bool DNAUSB::sendEnterBootloader( hid_device* device )
{
	return sendCommand( device, USBCommandEnterBootloader );
}

//------------------------------------------------------------------------------
bool DNAUSB::sendCode( hid_device* device, const unsigned char* code, const unsigned int size, unsigned char *checksum )
{
	unsigned char codePage[66] = { Report_Command, USBCommandCodePage };

	if ( !checksum )
	{
		return false;
	}

	*checksum = 0;
	for( unsigned int i=0; i<size; i++ )
	{
		*checksum += code[i];
	}

	for( unsigned int pos = 0; pos < size; pos += 64 )
	{
		memcpy( codePage + 2, code + pos, (64 > size) ? size - pos : 64 );

		//	asciiDump( codePage + 2, 64 );

		if ( hid_send_feature_report(device, codePage, 66) == -1 )
		{
			Log( "failed to send code page[%d]", pos / 64);
			return false;
		}
	}
	
	return true;
}

//------------------------------------------------------------------------------
bool DNAUSB::sendData( hid_device* device, const unsigned char data[64] )
{
	unsigned char command[66] = { Report_Command, USBCommandWriteData };
	memcpy( command + 2, data, 64 );

	if ( hid_send_feature_report(device, command, 66) == -1 )
	{
		Log( "failed to send data" );
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------
bool DNAUSB::getData( hid_device* device, unsigned char data[64] )
{
	unsigned char message[66] = { Report_Command };
	for(;;)
	{
		if ( hid_get_feature_report(device, message, 66) == -1 )
		{
			Log( "failed to poll for available data" );
			return false;
		}

		if ( message[1] & DNA_RTS )
		{
			break;
		}

//		asciiDump( message, 66 );
		usleep( 10000 );
	}

	memcpy( data, message + 2, 64 );

	return true;
}

//------------------------------------------------------------------------------
void DNAUSB::setLoggingCallback( void (*callback)(const char* msg) )
{
	Log.setCallback( callback );
}


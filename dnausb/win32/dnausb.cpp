#include "../dnausb.h"
#include "../../firmware/dna/dna_defs.h"

#include <Setupapi.h>

extern "C"
{
#include <api/hidsdi.h>
}

#include "../../util/str.hpp"
#include "../../util/simple_log.hpp"
#include "../../util/asciidump.hpp"


#define D_LOG
#define D_OPEND(a) a


static SimpleLog Log;


//------------------------------------------------------------------------------
HANDLE DNAUSB::openDevice( int vid, int pid, const char* vendor, char* product )
{
	HANDLE handle = INVALID_HANDLE_VALUE;

	// from DDK
	const GUID GUID_DEVINTERFACE_HID = { 0x4D1E55B2L, 0xF16F, 0x11CF, { 0x88, 0xCB, 0x00, 0x11, 0x11, 0x00, 0x00, 0x30 } };

	HANDLE deviceSet = SetupDiGetClassDevs( &GUID_DEVINTERFACE_HID, NULL, NULL, DIGCF_PRESENT | DIGCF_INTERFACEDEVICE );

	if ( deviceSet == INVALID_HANDLE_VALUE )    
	{
		return handle;
	}    

	SP_INTERFACE_DEVICE_DATA interfaceInfo;    
	SP_DEVINFO_DATA deviceInfoData;
	interfaceInfo.cbSize = sizeof( interfaceInfo );
	deviceInfoData.cbSize = sizeof( SP_DEVINFO_DATA );
	BYTE buf[1024];
	DWORD size;

	int i=0;
	if( SetupDiEnumDeviceInfo(deviceSet, i++, &deviceInfoData) )
	{
		DWORD interfaceNumber = 0;
		while ( SetupDiEnumDeviceInterfaces( deviceSet,
											 0, 
											 &GUID_DEVINTERFACE_HID,
											 interfaceNumber++,
											 &interfaceInfo) )
		{
			SP_DEVICE_INTERFACE_DETAIL_DATA *details = (SP_DEVICE_INTERFACE_DETAIL_DATA *)buf;
			details->cbSize = sizeof (SP_DEVICE_INTERFACE_DETAIL_DATA);

			size = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA) + 256;

			if ( !SetupDiGetDeviceInterfaceDetail( deviceSet, 
												   &interfaceInfo,
												   details,
												   size,
												   0,
												   0 ) )
			{
				Log( "SetupDiGetDeviceInterfaceDetail failed [%d]", GetLastError() );
				continue;
			}

			Cstr temp;
			temp.toChar( details->DevicePath );
			temp.toLower();
			Log( "Considering [%s]", temp.c_str() );

			const char* vidStr = strstr( temp.c_str(), "vid" );
			if ( !vidStr )
			{
				Log( "vid not found [%s]", temp.c_str() );
				continue;
			}
			vidStr += 3;

			for( ; *vidStr && !isxdigit(*vidStr); vidStr++ );

			if ( !vidStr )
			{
				Log( "could not find valid vid [%s]", temp.c_str() );
				continue;
			}

			if ( strtol(vidStr, 0, 16) != vid )
			{
				continue;
			}

			const char* pidStr = strstr( temp.c_str(), "pid" );
			if ( !pidStr )
			{
				Log( "pid not found [%s]", temp.c_str() );
				continue;
			}
			pidStr += 3;

			for( ; *pidStr && !isxdigit(*pidStr); pidStr++ );

			if ( !pidStr )
			{
				Log( "could not find valid pid [%s]", temp.c_str() );
				continue;
			}

			if ( strtol(pidStr, 0, 16) != pid )
			{
				continue;
			}

			int openFlag = 0;  // may be FILE_FLAG_OVERLAPPED
			handle = CreateFile( details->DevicePath, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, openFlag, NULL );
			if ( handle == INVALID_HANDLE_VALUE )
			{
				Log( "could not open device" );
				continue;
			}

			if( !HidD_GetManufacturerString(handle, buf, 1024) )
			{
				CloseHandle( handle );
				handle = INVALID_HANDLE_VALUE;
				Log( "could not fetch manufacturer string" );
				continue;
			}

			temp.toChar( (wchar_t*)buf );
			Log( "manufacturer [%s]", temp.c_str() );

			if ( !temp.isMatch(vendor, false) )
			{
				CloseHandle( handle );
				handle = INVALID_HANDLE_VALUE;
				continue;
			}

			HidD_GetProductString( handle, buf, 1024 );
			temp.toChar( (wchar_t*)buf );

			strcpy( product, temp.c_str() );

			break;
		}
	}

	SetupDiDestroyDeviceInfoList( deviceSet );
	return handle;
}

//------------------------------------------------------------------------------
void DNAUSB::closeDevice( DNADEVICE device )
{
	CloseHandle( device );
}

//------------------------------------------------------------------------------
bool DNAUSB::getProductId( HANDLE device, unsigned char *id )
{
	char packet[66] = { Report_Command };	
	
	if ( !HidD_GetFeature(device, packet, 66) )
	{
		Log( "failed to get status w/%d", GetLastError() );
		return false;
	}

	*id = packet[1] & DNA_PRODUCT_MASK;

	return true;
}

//------------------------------------------------------------------------------
static bool sendCommand( DNADEVICE device, const unsigned char cmd, const unsigned char param =0 )
{
	unsigned char comm[66] = { Report_Command, cmd, param };
	if ( !HidD_SetFeature(device, comm, 66) &&  GetLastError() != 31 )
	{
		Log( "failed to send command [%d] w/%d", (int)cmd, GetLastError() );
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------
bool DNAUSB::sendEnterApp( DNADEVICE device, unsigned char checksum )
{
	return sendCommand( device, USBCommandEnterApp, checksum );
}

//------------------------------------------------------------------------------
bool DNAUSB::sendEnterBootloader( DNADEVICE device )
{
	return sendCommand( device, USBCommandEnterBootloader );
}

//------------------------------------------------------------------------------
bool DNAUSB::sendCode( HANDLE device, const unsigned char* code, const unsigned int size, unsigned char *checksum )
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
	
		if ( !HidD_SetFeature(device, codePage, 66) &&  GetLastError() != 31 )
		{
			Log( "failed to send code page[%d] w/%d", pos / 64, GetLastError() );
			return false;
		}
	}

	return true;
}

//------------------------------------------------------------------------------
bool DNAUSB::sendData( HANDLE device, const unsigned char data[64] )
{
	unsigned char command[66] = { Report_Command, USBCommandWriteData };
	memcpy( command + 2, data, 64 );

	if ( !HidD_SetFeature(device, command, 66) &&  GetLastError() != 31 )
	{
		Log( "failed to send command w/%d", GetLastError() );
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------
bool DNAUSB::getData( HANDLE device, unsigned char data[64] )
{
	char message[66] = { Report_Command };
	for(;;)
	{
		if ( !HidD_GetFeature(device, message, 66) )
		{
			Log( "failed to poll for available data w/%d", GetLastError() );
			return false;
		}

		if ( message[1] & DNA_RTS )
		{
			break;
		}

//		asciiDump( message, 66 );
		Sleep(100);
	}
	
	memcpy( data, message + 2, 64 );
	
	return true;
}

//------------------------------------------------------------------------------
void DNAUSB::setLoggingCallback( void (*callback)(const char* msg) )
{
	Log.setCallback( callback );
}

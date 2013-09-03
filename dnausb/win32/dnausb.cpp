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


static unsigned int s_lastCommandTimestamp = 0;


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

			Sleep(10);

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
void waitForCommandInterval()
{
	unsigned int current = GetTickCount();
//	if ( (s_lastCommandTimestamp + 10) > current )
	{
		Sleep(10);
	}
	
	s_lastCommandTimestamp = GetTickCount();
}

//------------------------------------------------------------------------------
bool DNAUSB::getProductId( HANDLE device, unsigned char *id )
{
	char packet[66] = { Report_Command };	

	waitForCommandInterval();
	
	if ( !HidD_GetFeature(device, packet, 7) )
	{
		Log( "failed to get status w/%d", GetLastError() );
		return false;
	}

	Log( "product id status: [%d]\n", GetLastError() );

	*id = packet[1] & DNA_PRODUCT_MASK;

	return true;
}

//------------------------------------------------------------------------------
static bool sendCommand( DNADEVICE device, const unsigned char cmd, const unsigned char param =0 )
{
	unsigned char comm[66] = { Report_Command, cmd, param };
	waitForCommandInterval();
	if ( !HidD_SetFeature(device, comm, 66) &&  GetLastError() != 31 )
	{
		Log( "failed to send command [%d] w/%d", (int)cmd, GetLastError() );
		return false;
	}

	Log( "command status: [%d]\n", GetLastError() );

	return true;
}

//------------------------------------------------------------------------------
bool DNAUSB::sendEnterApp( DNADEVICE device )
{
	return sendCommand( device, USBCommandEnterApp );
}

//------------------------------------------------------------------------------
bool DNAUSB::sendEnterBootloader( DNADEVICE device )
{
	return sendCommand( device, USBCommandEnterBootloader );
}

//------------------------------------------------------------------------------
bool sendWithRetry( DNADEVICE device, unsigned char *command )
{
	unsigned char status[8] = { Report_Command };	
	do
	{
		waitForCommandInterval();
		if ( !HidD_SetFeature(device, command, 7) && GetLastError() != 31 )
		{
			Log( "failed to send w/retry w/%d", GetLastError() );
			return false;
		}

		waitForCommandInterval();
		if ( !HidD_GetFeature(device, status, 7) )
		{
			Log( "failed to get return status w/%d", GetLastError() );
			return false;
		}

		if ( status[2] )
		{
			Log( "command failure, retrying" );
		}
		
	} while( status[2] );

	return true;
}


//------------------------------------------------------------------------------
bool DNAUSB::sendCode( HANDLE device, const unsigned char* code, const unsigned int size )
{
	unsigned char codePage[8] = { Report_Command };

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

			if ( !HidD_SetFeature(device, codePage, 7) && GetLastError() != 31 )
			{
				Log( "failed to sendCode <1> w/%d", GetLastError() );
				return false;
			}
		}

		Log( "page[%d/%d]", pos / 64, size / 64 );

		codePage[1] = BootloaderCommandCommitPage;
		*(unsigned short *)(codePage + 2) = address;
		address += 64;

		if ( !HidD_SetFeature(device, codePage, 7) && GetLastError() != 31 )
		{
			Log( "failed to sendCode <1> w/%d", GetLastError() );
			return false;
		}

		Sleep( 10 );
	}

	pos = 0;
	for( int i=0; i<32; i += 2 )
	{
		codePage[1] = BootloaderCommandLoadPage;
		
		if ( pos < size )
		{
			codePage[1] = i*2;
			codePage[2] = code[pos++];
			codePage[3] = code[pos++];
			codePage[4] = code[pos++];
			codePage[5] = code[pos++];
		}

		if ( !HidD_SetFeature(device, codePage, 7) && GetLastError() != 31 )
		{
			Log( "failed to sendCode <1> w/%d", GetLastError() );
			return false;
		}
	}

	codePage[1] = BootloaderCommandCommitPage;
	codePage[2] = 0;
	codePage[3] = 0;
	*(unsigned short *)(codePage + 4) = size;
	codePage[6] = 0;
	for( int j=0; j<size; j++ )
	{
		codePage[6] += code[j];
	}

	if ( !HidD_SetFeature(device, codePage, 7) && GetLastError() != 31 )
	{
		Log( "failed to sendCode <1> w/%d", GetLastError() );
		return false;
	}

	Log( "comitted zero page" );

	Sleep( 10 );

	return true;
}

//------------------------------------------------------------------------------
bool DNAUSB::sendData( HANDLE device, const unsigned char data[64] )
{
	unsigned char command[66] = { Report_Command, USBCommandWriteData };
	memcpy( command + 2, data, 6 );

	waitForCommandInterval();
	if ( !HidD_SetFeature(device, command, 8) &&  GetLastError() != 31 )
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
		waitForCommandInterval();
		if ( !HidD_GetFeature(device, message, 8) )
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

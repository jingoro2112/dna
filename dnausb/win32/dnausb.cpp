/* Copyright: (c) 2013 by Curt Hartung
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#include "../dnausb.h"
#include "../../firmware/dna/dna_defs.h"
#include "../../util/simple_log.hpp"
#include "../../util/str.hpp"

#include <Setupapi.h>
extern "C"
{
#include <api/hidsdi.h>
}

extern SimpleLog Log;


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
bool DNAUSB::HID_GetFeature( DNADEVICE device, unsigned char* buf, unsigned int len )
{
	return HidD_GetFeature( device, buf, len ) ? true : false;
}

//------------------------------------------------------------------------------
bool DNAUSB::HID_SetFeature( DNADEVICE device, unsigned char* buf, unsigned int len )
{
	return (HidD_SetFeature(device, buf, len) ? true : false) || GetLastError() == 31;
}

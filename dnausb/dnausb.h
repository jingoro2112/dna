#ifndef USBUTIL_H
#define USBUTIL_H
/*------------------------------------------------------------------------------*/
/* Copyright: (c) 2013 by Curt Hartung
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#ifdef _WIN32
#include <windows.h>
#define DNADEVICE HANDLE
#define INVALID_DNADEVICE_VALUE INVALID_HANDLE_VALUE

#else
//#include <libusb.h>
//#define DNADEVICE libusb_device_handle*
//#define INVALID_DNADEVICE_VALUE 0

#include "linux/hidapi/hidapi.h"
#define DNADEVICE hid_device*
#define INVALID_DNADEVICE_VALUE 0

#endif

namespace DNAUSB
{

DNADEVICE openDevice( int vid, int pid, const char* vendor, char* product );
void closeDevice( DNADEVICE device );
	
bool getProductId( DNADEVICE device, unsigned char *id );

bool sendEnterBootloader( DNADEVICE device );

// used just for the bootloader
bool sendCode( DNADEVICE device, const unsigned char* code, const unsigned int size );

bool sendData( DNADEVICE device, const unsigned char* data, const unsigned char size );
bool getData( DNADEVICE device, unsigned char* data, unsigned char* size =0 );

bool sendCommand( DNADEVICE device, const unsigned char command, const unsigned char data[5] =0 );

bool HID_GetFeature( DNADEVICE device, unsigned char* buf, unsigned int len );
bool HID_SetFeature( DNADEVICE device, unsigned char* buf, unsigned int len );

};

#endif

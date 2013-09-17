/* Copyright: (c) 2013 by Curt Hartung avr@northarc.com
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#ifndef USBUTIL_H
#define USBUTIL_H

#ifdef _WIN32
#include <winsock2.h>
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

DNADEVICE openDevice( const int vid, const int pid, const char* vendor, char* product =0 );
void closeDevice( DNADEVICE device );
	
bool getProductId( DNADEVICE device, unsigned char *id, unsigned char *version =0 );

bool sendEnterBootloader( DNADEVICE device );

// used just for the bootloader
bool sendCode( const int vid, const int pid, const char* vendor,
			   const unsigned char* code,
			   const unsigned int size,
			   void (*status)(const unsigned int percent) /*=0*/ );

bool sendData( DNADEVICE device, const unsigned char* data, const unsigned char size );
bool getData( DNADEVICE device, unsigned char* data, unsigned char* size =0, unsigned char sizeExpected =0 );

bool sendCommand( DNADEVICE device, const unsigned char command, const unsigned char data[5] =0 );

bool HID_GetFeature( DNADEVICE device, unsigned char* buf, unsigned int len );
bool HID_SetFeature( DNADEVICE device, unsigned char* buf, unsigned int len );

};

#endif

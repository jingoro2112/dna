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

#define DNAUSB_DATA_SIZE 64

namespace DNAUSB
{

DNADEVICE openDevice( int vid, int pid, const char* vendor, char* product );
void closeDevice( DNADEVICE device );
	
bool getProductId( DNADEVICE device, unsigned char *id );

bool sendEnterApp( DNADEVICE device, unsigned char checksum );
bool sendEnterBootloader( DNADEVICE device );

bool sendCode( DNADEVICE device, const unsigned char* code, const unsigned int size, unsigned char *checksum );

bool sendData( DNADEVICE device, const unsigned char data[64] );
bool getData( DNADEVICE device, unsigned char data[64] );

void setLoggingCallback( void (*callback)(const char* msg) );

};

#endif

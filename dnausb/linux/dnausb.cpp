/* Copyright: (c) 2013 by Curt Hartung avr@northarc.com
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#include "../dnausb.h"

#include "../../firmware/dna/dna_defs.h"
#include "../../util/simple_log.hpp"
#include "../../util/str.hpp"

extern SimpleLog Log;

//------------------------------------------------------------------------------
hid_device* DNAUSB::openDevice( int vid, int pid, const char* vendor, char* product )
{
	hid_exit(); // force a re-enumeration, seems to bre required to properly re-connect to a device we have restarted

	hid_init();

	hid_device* device = hid_open( vid, pid, NULL);
	if ( !device )
	{
		Log( "could not open device" );
		return 0;
	}

	char buf[512];
	Cstr temp;
	if ( hid_get_manufacturer_string( device, buf, 512) )
	{
		Log( "could not fetch manufacturer string");
		hid_close( device );
		return 0;
	}

	if ( strcmp( vendor, buf) )
	{
		Log( "no match [%s]", vendor );
		hid_close( device );
		return 0;
	}

	if ( product && hid_get_product_string( device, product, 64) )
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
bool DNAUSB::HID_GetFeature( DNADEVICE device, unsigned char* buf, unsigned int len )
{
	return hid_get_feature_report( device, buf, len );
}

//------------------------------------------------------------------------------
bool DNAUSB::HID_SetFeature( DNADEVICE device, unsigned char* buf, unsigned int len )
{
	return hid_send_feature_report( device, buf, len );
}

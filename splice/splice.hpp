#ifndef LOADER_HPP
#define LOADER_HPP
/* ------------------------------------------------------------------------- */
/* Copyright: (c) 2013 by Curt Hartung
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#if defined(_WIN32) && defined(MFC_GUI)
#include <stdafx.h>
#endif

#include "../dnausb/dnausb.h"

const int c_majorVersion = 1;
const int c_minorVersion = 1;


//------------------------------------------------------------------------------
namespace Splice
{

// reutrns % the image will fill, 0 for error
unsigned int checkDNAImage( const unsigned char* image, const unsigned int len, char* err =0 );
char* stringFromId( const unsigned char id, char* buf );
bool proxyRNA( DNADEVICE device,
			   const unsigned char header, // first 8 bits of USB data, to alert end-app what is coming
			   const unsigned char target, // RNA target
			   const unsigned char command, // RNA Command
			   const void* data =0, const int size = 0 ); // optional data payload

}
   
#endif
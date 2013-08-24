#ifndef LOADER_HPP
#define LOADER_HPP
/* ------------------------------------------------------------------------- */
/* Copyright: (c) 2013 by Curt Hartung
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */


#include "../dnausb/dnausb.h"

//------------------------------------------------------------------------------
namespace Loader
{

bool checkDNAImage( const unsigned char* image, const unsigned int len, char* err =0 );
DNADEVICE openDevice( char* product =0 );

}
   
#endif
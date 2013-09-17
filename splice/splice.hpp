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

//------------------------------------------------------------------------------
namespace Splice
{

// reutrns % the image will fill, 0 for error
unsigned int checkDNAImage( const unsigned char* image, const unsigned int len, char* err =0 );
char* stringFromId( const unsigned char id, char* buf );

}
   
#endif
#ifndef ASCIIDUMP_HPP
#define ASCIIDUMP_HPP
/*------------------------------------------------------------------------------*/
/* Copyright: (c) 2013 by Curt Hartung
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#include "str.hpp"

//------------------------------------------------------------------------------
inline const char* asciiDump( const char* data, unsigned int len, Cstr* str =0 )
{
	Cstr local;
	Cstr *use = str ? str : &local;
	use->clear();
	for( unsigned int i=0; i<len; i++ )
	{
		use->appendFormat( "0x%08X: ", i );
		char dump[24];
		unsigned int j;
		for( j=0; j<16 && i<len; j++, i++ )
		{
			dump[j] = isgraph((unsigned char)data[i]) ? data[i] : '.';
			dump[j+1] = 0;
			use->appendFormat( "%02X ", (unsigned char)data[i] );
		}

		for( ; j<16; j++ )
		{
			use->appendFormat( "   " );
		}
		i--;
		*use += ": ";
		*use += dump;
		*use += "\n";
	}

	if ( !str )
	{
		printf( "%s\n", use->c_str() );
	}

	return str ? str->c_str() : 0;
}

#endif

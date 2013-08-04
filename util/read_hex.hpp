#ifndef READ_HEX_HPP
#define READ_HEX_HPP
/*------------------------------------------------------------------------------*/
/* Copyright: (c) 2013 by Curt Hartung
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#include "linklist.hpp"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

namespace ReadHex
{
	
//------------------------------------------------------------------------------
struct Chunk
{
	int addressBase;
	char data[65545];
	int size;
};

//------------------------------------------------------------------------------
bool parse( CLinkList<Chunk>& chunklist, const char* data, const int size )
{
	if ( !data )
	{
		return false;
	}

	chunklist.clear();
	Chunk* chunk = 0;

	int pos = 0;
	char buf[5];

	while( pos < size )
	{
		for( ; data[pos] != ':' && pos < size; pos++ );
		if ( pos >= size )
		{
			printf( "':' unfound\n" );
			break;
		}
		pos++;

		if ( (size - pos) < 10 )
		{
			printf( "size - pos = %d\n", size - pos );
			break;
		}

		buf[0] = data[pos++];
		buf[1] = data[pos++];
		buf[2] = 0;
		int byteCount = strtol( buf, 0, 16 );

		buf[0] = data[pos++];
		buf[1] = data[pos++];
		buf[2] = data[pos++];
		buf[3] = data[pos++];
		buf[4] = 0;
		int offsetAddress = strtol( buf, 0, 16 );

		buf[0] = data[pos++];
		buf[1] = data[pos++];
		buf[2] = 0;
		int recordType = strtol( buf, 0, 16 );
	

		switch( recordType )
		{
			case 0:
			{
				if ( !chunk )
				{
					chunk = chunklist.add();
					chunk->addressBase = offsetAddress;
					memset( chunk->data, 0, sizeof(chunk->data) );
					chunk->size = 0;
				}

				buf[2] = 0;

				for( int i=0; i<byteCount; i++ )
				{
					if ( !(buf[0] = data[pos++]) || !(buf[1] = data[pos++]) )
					{
						goto parse_fail;
					}

					int offset = (offsetAddress + i) - chunk->addressBase;
					if ( offset >= chunk->size )
					{
						chunk->size = offset + 1;
					}
					chunk->data[offset] = (char)strtol( buf, 0, 16 );

//					printf( "[%d/%d/%d %02X]", i, byteCount, offset, (unsigned char)chunk->data[offset] );
				}

//				printf( "\n" );

				break;
			}
			
			case 1:
			{
				return true; // reached EOF, only legal exit point
			}
			
			case 2:
			case 3:
			case 4:
			case 5:
			{
				printf( "unhandled record type [%d]\n", recordType );
				break;
			}
			
			default:
			{
				printf( "unrecognized record type [%d]\n", recordType );
				goto parse_fail;
			}
		}
	}

parse_fail:
	chunklist.clear();
	return false;
}

};

#endif

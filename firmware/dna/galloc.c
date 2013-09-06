/* Copyright: (c) 2013 by Curt Hartung
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */
#include "galloc.h"

#include <stdlib.h>
#include <avr/io.h>

extern unsigned char __heap_start;
static unsigned char* s_heapStart = &__heap_start;
static unsigned char* s_firstFree = &__heap_start;

// blocks have 2-byte overhead:
// [size in blocks:8][handle:8]

//------------------------------------------------------------------------------
void ginitHeap( unsigned char* newHeap )
{
	s_heapStart = newHeap;
	s_firstFree = newHeap;
}

//------------------------------------------------------------------------------
unsigned int gramUsage()
{
	return s_firstFree - s_heapStart;
}

//------------------------------------------------------------------------------
unsigned char galloc( unsigned char size )
{
	// by definition no allocations have taken place, and there fore no
	// handles can be valid. this also has the effect of auto-initting
	// to keep things safe, at the cost of a few extra cycles
	if ( s_firstFree == s_heapStart ) 
	{
		s_firstFree[1] = 0x1;
	}
	
	unsigned char ret = s_firstFree[1]; // next handle is pre-allocated, prepare to return it
	s_firstFree[1] |= 0x80; // mark this block as allocated

	// set up the block size (including the 2-byte overhead) and jump
	// free pointer up to next free block
#if GENE_BLOCK_SIZE == 1
	s_firstFree[0] = size + 2; // bytes
	s_firstFree += s_firstFree[0];
#else
	s_firstFree[0] = size; // blocks
	s_firstFree += (unsigned int)s_firstFree[0] * GENE_BLOCK_SIZE + 2;
#endif

	// prepare the next handle
	s_firstFree[1] = ret + 1;
	s_firstFree[1] &= 0x7F; // wrap at 127 (and hope they don't have blasters)
	
	return ret;
}

//------------------------------------------------------------------------------
char* gdefragEx( unsigned char handle )
{
	// traverse table
looprestart:
	for( unsigned char *b = s_heapStart;
		 b < s_firstFree;
#if GENE_BLOCK_SIZE == 1
		 b += *b ) // jump forward 'size' bytes
#else
		 b += (unsigned int)*b * GENE_BLOCK_SIZE + 2 ) // jump forward 'blocks*blocksize + 2 bytes of overhead bytes
#endif
	{
		if ( handle ) // looking for a handle, not actually defragging
		{
			if ( (handle | 0x80) == *(b + 1) )
			{
				if ( handle & 0x80 )
				{
					goto unconditionalCondense; // looking to reap it
				}
				else
				{
					return (char *)(b + 2); // looking to return it
				}
			}
		}
		else
		{
#if GENE_BLOCK_SIZE == 1
			unsigned char size;
#else
			unsigned int size;
#endif
			unsigned char* from;
			unsigned char* to;

			if ( *s_firstFree == *b )
			{
				(*s_firstFree)++; // handle collision is unlikely but possible, make sure next handle returned is unique
				goto looprestart; // restart entire process to be sure we didn't just grab ANOTHER collided handle
			}

			if ( !(*b & 0x7F) ) // if this block was marked free, condense
			{
unconditionalCondense: 
				
#if GENE_BLOCK_SIZE == 1
				size = *b; // size is stored as size
#else
				size = *b * GENE_BLOCK_SIZE + 2; // size is stored as blocks, must compute bytes
#endif
				from = b + size; // starting at the next block
				to = b; // replace to here
				while( from < (s_firstFree + 2) )
				{
					*to++ = *from++;
				}

				s_firstFree -= size; // adjust free pointer down

				break; // by definition only a single block can be marked, this ain't garbage collection
			}
		}
	}

	return 0;
}

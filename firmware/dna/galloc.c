/* Copyright: (c) 2013 by Curt Hartung
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */
#include "galloc.h"

extern unsigned char __heap_start;
static unsigned char* s_heapStart = &__heap_start;
static unsigned char* s_firstFree = &__heap_start;

// blocks have 2-byte overhead:
// [size in blocks:8][unnused:1][handle:8]

//------------------------------------------------------------------------------
void grelocateHeap( unsigned char* newHeap )
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
	// by definition no allocations have taken place, and therefore no
	// handles are valid, so assume first-time alloc, set to 1 thus auto-initting
	// at the cost of a few extra cycles
	if ( s_firstFree == s_heapStart ) 
	{
		s_firstFree[1] = 0x1;
	}
	
	unsigned char ret = s_firstFree[1]; // next handle is pre-allocated, prepare to return it

	// set up the block size (including the 2-byte overhead) and jump
	// free pointer up to next free block
#if GENE_BLOCK_SIZE == 1
	s_firstFree[0] = size + 2; // bytes
	s_firstFree += s_firstFree[0];
#else
	s_firstFree[0] = size; // blocks
	s_firstFree += (unsigned int)s_firstFree[0] * GENE_BLOCK_SIZE + 2;
#endif

	if ( (s_firstFree[1] = ret + 1) & 0x80 )
	{
		s_firstFree[1] = 0x1;
	}

	// by asking for our own pointer (by definition the last in line)
	// as a side-effect the current 'free' handle is checked for
	// collision
	_gtraverse( ret );
	
	return ret;
}

//------------------------------------------------------------------------------
// this started as a defragger and sort of kept the name, but is now
// the all-purpose function. The reason is to only have the "traverse
// the alloc table" logic residing in one place in flash, thus keeping
// the code ultra-compact. Depending on context this function returns a
// handle, frees a handle, or 
char* _gtraverse( unsigned char handle )
{
	// traverse table
#if GENE_BLOCK_SIZE == 1
	unsigned char size;
#else
	unsigned int size;
#endif
	unsigned char* from;
	unsigned char* to;
	unsigned char currentFreeHandle;

looprestart:
	currentFreeHandle = s_firstFree[1];

	for( unsigned char *b = s_heapStart;
		 b < s_firstFree;
#if GENE_BLOCK_SIZE == 1
		 b += *b ) // jump forward 'size' bytes
#else
		 b += (unsigned int)*b * GENE_BLOCK_SIZE + 2 ) // jump forward 'blocks*blocksize + 2 bytes of overhead bytes
#endif
	 {
		if ( (handle & 0x7F) == *(b + 1) ) // match?
		{
			if ( !(handle & 0x80) ) // just wanted it returned?
			{
				return (char *)(b + 2);
			}

			// no, wanted to free it
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

			break; // no need to continue, a free cannot cause a collision
		}

		// check for handle collision
		if ( currentFreeHandle == *(b + 1) )
		{
			if ( ++currentFreeHandle & 0x80 )
			{
				currentFreeHandle = 0x1;
			}
			s_firstFree[1] = currentFreeHandle;
			goto looprestart; // restart entire process to be sure we didn't just install another collided handle
		}
	}

	return 0;
}

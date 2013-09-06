#ifndef GALLOC_H
#define GALLOC_H
/* Copyright: (c) 2013 by Curt Hartung avr@northarc.com
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

/*

 GENE Allocator

                                    OVERVIEW:
                                    
 I included malloc() to see what it would cost in terms of Flash and RAM and jeezlouise.
 this allocator is robust and SMALL. it trades RAM for a few extra
 processor cycles (a few!) while keeping the code size way down.

 resources: 4 bytes statically allocated RAM to track head and tail
              of heap
            2 bytes per heap allocation
            ~210 bytes Flash with a block size of 1 (any other size, ~240 bytes)

 limitations: maximum of 127 allocations total
              maximum 256 blocks per allocation


 When memory is freed it is condensed; fragmentation is not possible by definition. The
 price paid for this is all memory is accessed with handles.

 gmalloc( size ) allocates a block of memory, returns a handle suitable

 gfree( handle ) frees a handle, automatically condensing remaining RAM
                 NOTE: This means that all pointer return by
                 gpointer(handle) should be consided INVALID after any
                 gfree() call

 gpointer( handle ) Map a handle to a block of memory, guaranteed valid until
                    (and only until) any call is made to gfree()

 gitHeap( start ) locate heap in a place other than __heap_start
                  NOTE: This invalidates all existing allocations

 gramUsage() report how much RAM has been allocated

sample code:


// typical usage:
unsigned char handle1 = gmalloc( sizeof(SomeStruct) );
unsigned char handle2 = gmalloc( sizeof(SomeStruct) ); // get some RAM

SomeStruct *s1 = (SomeStruct *)gpointer( handle1 );
SomeStruct *s2 = (SomeStruct *)gpointer( handle2 ); // map to a structure

// do some stuff...
s1->thing1 = 2;
s1->thing2 = 4;
s2->thing1 = 10;
s2->thing2 = 20;

// free the first handle
gfree( handle1 );

// which invalidates all other pointers, be sure to re-get them. this
// is the 'gotcha' of galloc, and I welcome suggestions on how to get
// around it, but I don't think its possible. personally I consider it
// a small price to pay for the functionality
s2 = gpointer( handle2 );

// do some more stuff..
s2->thing1 = 40;

// and free it
gfree( handle2 );


// alternately, this is cumbersome but guaranteed to never get you:
handle1 = gmalloc( sizeof(SomeStruct) );

((SomeStruct *)gpointer(handle1))->thing1 = 20; // de-reference each time it is used
//....


*/


// Any value technically works, but powers of two make much nicer code.
// The 'special' value of 1 allows much tighter code, best to leave it
// unless you really really REALLY need larger allocations than 256 bytes
#define GENE_BLOCK_SIZE 1

void ginitHeap( unsigned char* heapStart ); // if you want to locate the heap elsewhere than __heap_start
unsigned int gramUsage(); // report how many bytes (including overhead) has been allocated
unsigned char galloc( unsigned char size ); // allocate a block of memory of 'size' bytes and return a handle to it
char* gdefragEx( unsigned char handle ); // internal utility method, do not call
#define gfree( handle ) gdefragEx((handle) | 0x80) // free the memory, tolerant of zero
#define gpointer( handle ) gdefragEx( handle ) // map a handle to a pointer, valid until ANY call is made to gfree()

#endif

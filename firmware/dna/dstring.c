/*------------------------------------------------------------------------------*
 * Copyright: (c) 2013 by Curt Hartung avr@northarc.com
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#include "stdarg.h"

#ifdef AVR
#include "avr/pgmspace.h"
#else
#define pgm_read_byte(p) *p
#endif

void dsprintfEx( char pspace, char *buf, const char *fmt, va_list list );

//------------------------------------------------------------------------------
void dsprintf( char* buf, const char* fmt, ... )
{
	va_list list;
	va_start( list, fmt );
	dsprintfEx( 0, buf, fmt, list );
	va_end( list );
}

//------------------------------------------------------------------------------
void dsprintf_P( char* buf, const char* fmt, ... )
{
	va_list list;
	va_start( list, fmt );
	dsprintfEx( 1, buf, fmt, list );
	va_end( list );
}

/*
Spec      Example             Expected Output

%d        ("[%d]", 123)       [123]
%Nd       ("[%4d]", 123)      [ 123]
%0Nd      ("[%04d]", 123)     [0123]
%-Nd      ("[%-4d]", 123)     [123 ]
%-0Nd     ("[%-04d]", 123)    [123 ]

%x        ("[%x]", 0xABC)     [abc]
%Nx       ("[%4x]", 0xABC)    [ abc]
%0Nx      ("[%04x]", 0xABC)   [0abc]
%-Nx      ("[%-4x]", 0xABC)   [abc ]
%-0Nx     ("[%-04x]", 0xABC)  [abc ]

%X        ("[%X]", 0xABC)     [ABC]
%NX       ("[%4X]", 0xABC)    [ ABC]
%0NX      ("[%04X]", 0xABC)   [0ABC]
%-NX      ("[%-4X]", 0xABC)   [ABC ]
%-0NX     ("[%-04X]", 0xABC)  [ABC ]

%b        ("[%b]", 0x25)       [100101]
          ("[%b]", 0 )         []
%Nb       ("[%7b]", 0x25)      [ 100101]
%0Nb      ("[%07b]", 0x25)     [0100101]
%-Nb      ("[%-7b]", 0x25)     [100101 ]
%-0Nb     ("[%-07b]", 0x25)    [100101 ]

%p

%%

%c

%s
%Ns
%-Ns

%S
%NS
%-NS

*/

//------------------------------------------------------------------------------
enum
{
	zeroPad = 1<<0,
	negativeJustify = 1<<1,
	secondPass = 1<<2,
	negativeSign = 1<<3,
};

//------------------------------------------------------------------------------
void dsprintfEx( char pspace, char *buf, const char *fmt, va_list list )
{
	unsigned char base;
	char bits;
	unsigned char padChar;
	char *ptr;
	unsigned char columns;
	unsigned char chars;

	unsigned int val;
	char buffer[18]; // for formatted ints

resetState:
	base = 16;
	bits = 0;
	ptr = buffer;
	padChar = ' ';
	columns = 0;
	chars = 0;

	for(;;)
	{
		char c = pspace ? pgm_read_byte( fmt ) : *fmt;

		fmt++;
		
		if ( !c )
		{
			*buf = 0;
			break;
		}

		if ( !(secondPass & bits) )
		{
			if ( c != '%' )
			{
				*buf++ = c; // unformatted text stream, just copy it in
			}
			else
			{
				bits |= secondPass; // from initial to acquire
			}
		}
		else
		{
			if( c >= '1' && c <= '9' ) // if it is a number, parse it in
			{
				columns *= 10;
				columns += c - '0';
			}
			else if ( c == '%' )
			{
				*buf++ = '%'; // literal %
				goto resetState;
			}
			else if ( c == '-' )
			{
				bits |= negativeJustify;
			}
			else if ( c == '0' )
			{
				bits |= zeroPad;
				padChar = '0';
			}
			else if ( c == 's' || c == 'S' )
			{
				ptr = va_arg( list, char* );

				// get the string length so it can be formatted, don't
				// copy it, just count it
				while ( ((c == 'S') ? pgm_read_byte(ptr) : *ptr) )
				{
					ptr++;
					chars++;
				}
				ptr -= chars;

				padChar = ' '; // in case some joker uses a 0 in their column spec
copyToString:
				
				if ( columns < chars )
				{
					columns = chars;
				}

				if ( !(bits & negativeJustify) )
				{
					while( columns > chars )
					{
						*buf++ = padChar;
						columns--;
					}
				}
				
				if ( bits & negativeSign )
				{
					*buf++ = '-';
				}

				// now actually copy it in
				while( chars )
				{
					// copyToString is used by ints as well, but none
					// of them use 'S' so this is okay to do here
					*buf++ = (c == 'S') ? pgm_read_byte(ptr) : *ptr;
					ptr++;
					chars--;
					columns--;
				}

				while( columns-- )
				{
					*buf++ = ' ';
				}

				goto resetState;
			}
			else if ( c == 'x' || c == 'X' || c == 'p' ) // hexadecimal or pointer (pointer is treated as 'X'
			{
				chars = 5; // expect up to 4 character + terminator

convertBasePre:
				val = va_arg( list, unsigned int ); // this call is kind of expensive, make it as little as possible
convertBase:

				ptr[chars--] = 0;
				for ( ; chars; chars-- ) // cheaper by 6 bytes to check for null and inc the pointer then check for < 0xFF wierd huh?
				{
					ptr[chars] = '0' + val % base;
					val /= base;
				}
				ptr++;
				
				// make a passw where we figure out how long the string
				// is, while at the same time convert any non-numeric
				// characters into their alphabetical equivilant (only
				// applies to hex, falls through on all other formats)
				for( ; ptr[chars]; chars++ )
				{
					if ( ptr[chars] > '9' )
					{
						ptr[chars] += (c == 'x') ? 39 : 7;
					}
				}

				// format the number for display taking into account
				// zero-padding, justification and column availability
				if ( !(bits & zeroPad) || columns < chars || (bits & negativeJustify) )
				{
					while( *ptr == '0' && (chars > 1) )
					{
						ptr++;
						chars--;
					}
				}

				goto copyToString;
			}
			else if ( c == 'u' || c == 'd' || c == 'i' )
			{
				val = va_arg( list, unsigned int );

				if ( (c != 'u') && (val & 0x8000) )
				{
					bits |= negativeSign;
					#ifdef AVR
					val = (val ^ 0xFFFF) + 1;
					#else
					val = (val ^ 0xFFFFFFFF) + 1;
					#endif
				}

				#ifndef AVR
				val &= 0xFFFF;
				#endif

				chars = 6;
				base = 10;
				goto convertBase;
			}
			else if ( c == 'b' ) // binary! nice to have
			{
				chars = 17;
				base = 2;
				goto convertBasePre;
			}
			else if ( c == 'c' )
			{
				*buf++ = va_arg( list, unsigned int );
				goto resetState;
			}
			else
			{
				goto resetState;
			}
		}
	}
}


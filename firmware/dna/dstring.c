/*------------------------------------------------------------------------------*
 * 2013 by Curt Hartung avr@northarc.com
 * share and enjoy

 history (oldest first)
 * blargg: Fixed bug where %10d format is treated as %1d, since code was ignoring all zeroes in column width.
 * blargg: Added dvsprintf() and dvsprintf_P(). Moved pspace arg to end to avoid machine code having to move
           arguments around in registers.
 * blargg: Made val unsigned short so it doesn't need to be masked when built on PC.
 * blargg: Negation of an unsigned is defined in C as subtracting it from 0, which is the same as inverting all
           bits and adding one as your code was doing. This eliminates the need to know the width of val, so
           val=-val works on PC or AVR.
 * blargg: Original code never used first char of buffer, so I shrunk it by one byte.
 * blargg: Eliminated unnecessary initializations of base, ptr, and chars in resetState. These make it harder for
           the compiler to do lifetime analysis.
 * blargg: % is already in c for a literal % (saves an ldi).
 * blargg: Have int printing use %s's string-length calculation, simplifying int code.
 * blargg: Simplify left/right justify code to just compare columns with chars on each iter.
 * blargg: Since we never truncate a string, we just print until the null character. No need to mess with chars or columns.
 * blargg: Now chars specifies with of integer. Doesn't count terminator anymore.
 * blargg: Int conversion code is consolidated into one loop that runs 'chars' times. It prints int right-to-left in buffer,
           stopping once it's written the first char of the buffer. It keeps track of the right-most non-zero digit.
           It converts to hex as it goes.

 * blargg: Moved declarations of variables only used by string/int handling to smaller scope.
 * blargg: Renamed bits to flags to more clearly reflect its purpose (bits sounds like it might be related to integer handling).
 * blargg: Renamed buf to out, for clarity. Before there was buf and buffer.
 * blargg: Moved string and int handling to end, to minimize distance since int gotos into string print.
 * blargg: Converted while + iter stmt loops to for loops.
 *
 * curt: shaved another 12 bytes off by removing va-arg() call from integer case
 * curt: added the alternate representation logic ('#')
 * curt: conditional compilation of unuecessary modules
 */

#include "dstring.h"
#include "stdarg.h"

#ifdef __AVR__
#include "avr/pgmspace.h"
#else
#define pgm_read_byte(p) *p
#endif

//------------------------------------------------------------------------------
DSFORMAT_RETURN_TYPE dsprintf( char* buf, const char* fmt, ... )
{
	va_list list;
	va_start( list, fmt );
	
#ifdef DSFORMAT_RETURN_SIZE
	int ret = dsprintfEx( buf, fmt, list, 0 );
	va_end( list );
	return ret;
#else
	dsprintfEx( buf, fmt, list, 0 );
	va_end( list );
#endif
}

//------------------------------------------------------------------------------
DSFORMAT_RETURN_TYPE dsprintf_P( char* buf, const char* fmt, ... )
{
	va_list list;
	va_start( list, fmt );
#ifdef DSFORMAT_RETURN_SIZE
	int ret = dsprintfEx( buf, fmt, list, 1 );
	va_end( list );
	return ret;
#else
	dsprintfEx( buf, fmt, list, 1 );
	va_end( list );
#endif
}

//------------------------------------------------------------------------------
enum
{
	zeroPad         = 1<<0,
	negativeJustify = 1<<1,
	secondPass      = 1<<2,
	negativeSign    = 1<<3,
	parsingSigned   = 1<<4,
	altRep			= 1<<5,
};

//------------------------------------------------------------------------------
DSFORMAT_RETURN_TYPE dsprintfEx( char *out, const char *fmt, va_list list, char pspace )
{
resetState:;

	#ifdef DSFORMAT_RETURN_SIZE
	char* startPos = out;
	#endif
	
	char flags  = 0;
	#ifdef DSFORMAT_COLUMNS
	char padChar = ' ';
	unsigned char columns = 0;
	#endif
	
	for(;;)
	{
		char c = pspace ? pgm_read_byte( fmt ) : *fmt;
		fmt++;

		if ( !c )
		{
			*out = 0;
			break;
		}

		if ( !(secondPass & flags) )
		{
			if ( c != '%' ) // literal
			{
				*out++ = c;
			}
			else // possibly % format specifier
			{
				flags |= secondPass;
			}
		}
		#ifdef DSFORMAT_COLUMNS
		else if ( c >= '0' && c <= '9' ) // width
		{
			columns *= 10;
			columns += c - '0';
			if ( !columns ) // leading zero
			{
				flags |= zeroPad;
				padChar = '0';
			}
		}
		#endif
		#ifdef DSFORMAT_ALT_REP
		else if ( c == '#' )
		{
			flags |= altRep;
		}
		#endif
		#ifdef DSFORMAT_COLUMNS
		else if ( c == '-' ) // left-justify
		{
			flags |= negativeJustify;
		}
		#endif
		#ifdef DSFORMAT_SINGLE_CHARACTER
		else if ( c == 'c' ) // character
		{
			*out++ = va_arg( list, unsigned int );
			goto resetState;
		}
		#endif
		else if ( c == '%' ) // literal %
		{
			*out++ = c;
			goto resetState;
		}
		else // string or integer
		{
			char buf [16+1]; // buffer for integer
			
			const char *ptr; // pointer to first char of integer

			#ifdef DSFORMAT_CAPITAL_S
			if ( c == 's' || c == 'S' ) // string
			#else
			if ( c == 's' ) // string
			#endif
			{
				ptr = va_arg( list, char* );

				#ifdef DSFORMAT_COLUMNS
				padChar = ' '; // in case some joker uses a 0 in their column spec
				#endif
				
copyToString:;
				
				// get the string length so it can be formatted, don't
				// copy it, just count it
				unsigned char len = 0;
				#ifdef DSFORMAT_CAPITAL_S
				for ( ; ((c == 'S') ? pgm_read_byte(ptr) : *ptr); ptr++ )
				#else
				for ( ; *ptr; ptr++ )
				#endif
				{
					len++;
				}
				ptr -= len;

				#ifdef DSFORMAT_COLUMNS
				// Right-justify
				if ( !(flags & negativeJustify) )
				{
					for ( ; columns > len; columns-- )
					{
						*out++ = padChar;
					}
				}
				#endif
				
				if ( flags & negativeSign )
				{
					*out++ = '-';
				}
				
				// Copy string (%S uses pgm read, all others use normal
				// read)
				#ifdef DSFORMAT_ALT_REP
				if ( flags & altRep || c == 'p' )
				{
					*out++ = '0';
					*out++ = 'x';
				}
				#endif
				
				#ifdef DSFORMAT_CAPITAL_S
				char d;
				while ( (d = (c == 'S') ? pgm_read_byte(ptr++) : *ptr++) )
				{
					*out++ = d;
				}
				#else
				while ( (c = *ptr++) )
				{
					*out++ = c;
				}
				#endif
				
				#ifdef DSFORMAT_COLUMNS
				// Left-justify
				for ( ; columns > len; columns-- )
				{
					*out++ = ' ';
				}
				#endif
				
				goto resetState;
			}
			else
			{
				unsigned char base;
				unsigned char width;
				unsigned short val;
				
				if ( c == 'd' || c == 'i' )
				{
					flags |= parsingSigned;
					goto parseDecimal;
				}
				else if ( c == 'u' ) // decimal
				{
parseDecimal:
					base  = 10;
					width = 5;
					goto convertBase;
				}
				#ifdef DSFORMAT_BINARY
				else if ( c == 'b' ) // binary
				{
					base  = 2;
					width = 16;
					goto convertBase;
				}
				#endif
				#ifdef DSFORMAT_OCTAL
				else if ( c == 'o' ) // octal
				{
					base  = 8;
					width = 5;
					goto convertBase;
				}
				#endif
				else if ( c == 'x' || c == 'X' || c == 'p' ) // hexadecimal or pointer (pointer is treated as 'X')
				{
					base = 16;
					width = 4;
convertBase:
					val = va_arg( list, unsigned int );

					if ( (flags & parsingSigned) && (val & 0x8000) )
					{
						flags |= negativeSign;
						val = -val;
					}
			
					// Convert to given base, filling buffer backwards from least to most significant
					char* p = buf + width;
					*p = 0;
					ptr = p; // keep track of one past left-most non-zero digit
					do
					{
						char d = val % base;
						val /= base;
				
						if ( d )
						{
							ptr = p;
						}
				
						d += '0';
						if ( d > '9' ) // handle bases higher than 10
						{
							d += 'A' - ('9' + 1);
							if ( c == 'x' ) // lowercase
							{
								d += 'a' - 'A';
							}
						}
					
						*--p = d;

					} while ( p != buf );
					
					ptr--; // was one past char we want
			
					goto copyToString;
				}
				else // invalid format specifier
				{
//					*out++ = '?'; // seems frivilous, even cursory testing finds formatting errors
					goto resetState;
				}
			}
		}
	}

	#ifdef DSFORMAT_RETURN_SIZE
	return out - startPos;
	#endif
}

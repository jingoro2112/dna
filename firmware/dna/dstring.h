#ifndef DSTRING_H
#define DSTRING_H
/*------------------------------------------------------------------------------*
 * 2013 by Curt Hartung <avr@northarc.com> and Shay Green <gblargg@gmail.com>
 * share and enjoy
 */

#include <stdarg.h>

// custom define the funcitonality you want. byte cost is
// approximate since the optimizer can be a bit unpredictable in the
// final equation, but it will be close
// base size: ~495 bytes
#define DSFORMAT_COLUMNS     // 98 bytes: column spcifiers %10d %-10d %04X %4s etc..
#define DSFORMAT_BINARY      // 12 bytes: %b printing binary
//#define DSFORMAT_OCTAL       // 10 bytes: octal
//#define DSFORMAT_CAPITAL_S   // 28 bytes: '%S' reading a pgm_space string
//#define DSFORMAT_ALT_REP     // 84 bytes: the '#' thingy which adds a 0x to pointers and hex
//#define DSFORMAT_SINGLE_CHARACTER // 24 bytes: %c for single characters
//#define DSFORMAT_RETURN_SIZE // 14 bytes: return the number of characters written

#ifdef DSFORMAT_RETURN_SIZE
#define DSFORMAT_RETURN_TYPE int
#else
#define DSFORMAT_RETURN_TYPE void
#endif
DSFORMAT_RETURN_TYPE dsprintfEx( char *buf, const char *fmt, va_list list, char pspace );

DSFORMAT_RETURN_TYPE dsprintf( char* buf, const char* fmt, ... );
DSFORMAT_RETURN_TYPE dsprintf_P( char* buf, const char* fmt, ... );
#define dvsprintf( buf, fmt, args ) dsprintfEx( buf, fmt, args, 0 )
#define dvsprintf_D( buf, fmt, args ) dsprintfEx( buf, fmt, args, 1 )

#endif
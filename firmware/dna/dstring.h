#ifndef DSTRING_H
#define DSTRING_H
/*------------------------------------------------------------------------------*
 * 2013 by Curt Hartung avr@northarc.com
 * share and enjoy
 */

#include <stdarg.h>

void dsprintf( char* buf, const char* fmt, ... );
void dsprintf_P( char* buf, const char* fmt, ... );
void dvsprintf( char* buf, const char* fmt, va_list args );
void dvsprintf_P( char* buf, const char* fmt, va_list args );

#endif
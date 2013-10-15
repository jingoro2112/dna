#ifndef DSTRING_H
#define DSTRING_H
/*------------------------------------------------------------------------------*
 * Copyright: (c) 2013 by Curt Hartung avr@northarc.com
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

// 694 bytes
// sprintf w/-u,vfprintf -lprintf_min 1138
// sprintf w/o 1478

void dsprintf( char* buf, const char* fmt, ... );
void dsprintf_P( char* buf, const char* fmt, ... );

#endif
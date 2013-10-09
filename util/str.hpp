#ifndef STR_HPP
#define STR_HPP
/* ------------------------------------------------------------------------- */
/* Copyright: (c) 2013 by Curt Hartung
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#ifdef _WIN32
#pragma warning (disable : 4996) // remove Windows nagging about using _strincmp() etc
#else
#include <wchar.h>
#endif

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

/*

 Yes. yet another string class. Why is this one better than all the
 rest? For the typical reason: I wrote it myself.

 In real terms this string class collects a lot of functions that have
 been needed on the projects it has been brought to, specifically:

 - In-place and appended sprintf-style formatting, very handy for
 building dynamic strings such as SQL queries.

 - No default dynamic representation. Put a Cstr on the stack without
 sweating what the constructor will go off and do. Until the
 string gets larger than: */
const unsigned int c_sizeofBaseString = 32;
/*
 after which a new/delete load is imposed for obvious reasons.

 - Null termination not required. This class handles a string of
 anything, and all of the obvious methods support NULLs in the string.
 internally, the buffer carries an additional byte which it will
 terminate with a null for safety.

 - optimized single-character append. This comes in handy, if you
 append a single character the overhead is super-low, no
 mem-cpying/new/delete BS for one stinking character.

 - fileToBuffer and bufferToFile again quite handy

 - token parsing

 - caseless and partial matching

 - in-place string escaping

 - all of the data/length safety and operator overloading expected of a
 'real' string class.

  The idea is none of this functionality imposes a bunch of data being
 carried or newed for every Cstr created, I wanted a class that could
 be created without fear of a kernel-dive and sun-moon-stars data/text
 overhead being sucked in.

*/

#include <string.h>
#include <ctype.h>
#include <assert.h>

//-----------------------------------------------------------------------------
class Cstr
{
public:
	Cstr() { m_len = 0; m_smallbuf[0] = 0; m_str = m_smallbuf; m_buflen = c_sizeofBaseString; }
	Cstr( const Cstr& str ) { m_str = m_smallbuf; m_len = 0; m_buflen = c_sizeofBaseString; setString(str); }
	Cstr( const char* buf ) { m_str = m_smallbuf; m_len = 0; m_buflen = c_sizeofBaseString; setString(buf); }
	Cstr( const char* buf, const unsigned int len ) { m_str = m_smallbuf; m_len = 0; m_buflen = c_sizeofBaseString; setString(buf, len); }
	inline Cstr( wchar_t* wcs );
	inline Cstr( unsigned int len );

	~Cstr() { if ( m_str != m_smallbuf ) delete[] m_str; }

	inline void clear();
	inline void reserve( unsigned int size );

	// take ownership of this dynamic memory (do not copy it, its
	// probobly big!) this will clobber any existing data
	inline void giveOwnership( char *dynamicMemory, unsigned int len = 0 );
	inline char* getOwnership( unsigned int *len =0 ); // returns new'ed buffer with data

	// getting a pointer to the string rep
	const char* c_str() const { return m_str; }
	const wchar_t* w_str() const { return m_wstr; }
	operator const void*() const { return m_str; }
	operator const char*() const { return m_str; }

	// UNSAFE!!
	char* p_str() { return m_str; }
	wchar_t* p_wstr() { return m_wstr; }
	// as long as you are doing things you shouldn't be.. 
	void updateSize() { m_len = (unsigned int)strlen( c_str() ); }
	
	unsigned int size() const { return m_len; }
	unsigned int length() const { return m_len; }
	unsigned int wlength() const { return (unsigned int)wcslen((wchar_t*)m_str); }

	// read in the file and fill this buffer in
	inline bool fileToBuffer( const char* fileName );
	inline bool bufferToFile( const char* fileName, bool append =false ) const;

	inline wchar_t* toWide( wchar_t *dest );
	inline char* toChar( const wchar_t* wcs );
	
	inline void setString( const char* buf, unsigned int len =0 );
	void setString( const Cstr& str ) { setString( str.c_str(), str.length() ); }

	inline void trim(); // trim the leading and trailing whitespace

	inline void truncate( unsigned int newLen, bool recoverMemory =false );

	void enqueue( const char* data, const unsigned int len ) { append(data, len); }
	inline unsigned int dequeue( const unsigned int bytes, Cstr *str =0 );

	// convert this string's case
	Cstr& toLower() { for( unsigned int u=0; u<m_len ; u++ ) { m_str[u] = tolower( m_str[u] ); } return *this; }
	Cstr& toUpper() { for( unsigned int u=0; u<m_len ; u++ ) { m_str[u] = toupper( m_str[u] ); } return *this; }

	inline bool replace( const char* oldstr, const char* newstr, bool ignoreCase =false );
	int replaceAll( const char* oldstr, const char* newstr, bool ignoreCase =false ) { int r=0; while( replace(oldstr, newstr,ignoreCase) ) { r++; } return r; }

	inline bool isMatch( const char* buf, const bool matchCase = true, const bool matchPartial =false ) const;
	inline bool isWildMatch( const char* pattern, const bool matchCase =true ) const;
	inline int find( const char* buf, bool matchCase =true ) const;

	//	Note this allocates a 64k temporary buffer to format the string
	//	into. The resulting	string cannot be longer than this.
	inline const char* format( const char* format, ... );
	inline const char* format( const char* format, va_list arg );

	// appends use a growth algorithm so they are pretty effecient,
	// with a highly optimized single-character implementation
	inline const char* append( const void* vbuf, unsigned int len );
	const char* append( const char* buf ) { return append( buf, 0 ); }
	inline const char* append( const char c );
	const char* append( const Cstr &s ) { return append( s.c_str(), s.length() ); }
	const char* append( const Cstr &s, unsigned int len ) { return append( s.c_str(), len ); }
	inline const char* appendFormat( const char* format, ... );
	inline const char* appendFormat( const char* format, va_list arg );

	// installs the requested token into the passed Cstr
	inline bool getToken( unsigned int tokenNum, Cstr& token );

	Cstr& operator = ( const char* b ) { setString( b ); return *this; }
	Cstr& operator = ( const Cstr& S ) { setString( S ); return *this; }
	Cstr& operator = ( const char& c ) { append( c ); return *this; }
	Cstr& operator = ( const int& i ) { format( "%d", i ); return *this; }
	Cstr& operator = ( const unsigned int& u ) { format( "%u", u ); return *this; }
	Cstr& operator = ( const float& f ) { format( "%f", f ); return *this; }

	Cstr& operator += ( const Cstr& str ) { append(str.c_str(), str.length()); return *this; }
	Cstr& operator += ( const char* s ) { append(s); return *this; }
	Cstr& operator += ( char ch ) { append(ch); return *this; }

	char& operator[]( int l ) const { assert( (unsigned int)l<=m_len ); return m_str[l]; }
	char& operator[]( unsigned int l ) const { assert( l<=m_len ); return m_str[l]; }

	friend Cstr operator + ( const Cstr& s, const char* buf ) { Cstr r(s); r.append( buf ); return r; }
	friend Cstr operator + ( const Cstr& s1, const Cstr& s2 ) { Cstr r(s1); r.append( s2 ); return r; }

	friend bool operator == ( const Cstr &s1, const Cstr &s2 ) { return s1.m_len == s2.m_len && (memcmp(s1.c_str(), s2.c_str(), s1.m_len) == 0); }
	friend bool operator != ( const Cstr &s1, const Cstr &s2 ) { return s1.m_len != s2.m_len || (memcmp(s1.c_str(), s2.c_str(), s1.m_len) != 0); }
	friend bool operator == ( const char *z, const Cstr &s ) { return z && (strlen(z) == s.m_len) && !strcmp(s, z); }
	friend bool operator != ( const char *z, const Cstr &s ) { return !z || (strlen(z) != s.m_len) || strcmp(s, z); }
	friend bool operator == ( const Cstr &s, const char *z ) { return z && (strlen(z) == s.m_len) && !strcmp(s, z); }
	friend bool operator != ( const Cstr &s, const char *z ) { return !z || (strlen(z) != s.m_len) || strcmp(s, z); }

private:

	operator char*() const { return m_str; } // ohnoyadont

	union
	{
		char *m_str;
		wchar_t *m_wstr;
	};
	unsigned int m_len;
	unsigned int m_buflen;
	char m_smallbuf[ c_sizeofBaseString + 1 ];
};

//------------------------------------------------------------------------------
void Cstr::clear()
{
	if ( m_str != m_smallbuf )
	{
		delete[] m_str;
		m_str = m_smallbuf;
	}

	m_smallbuf[0] = 0;
	m_len = 0;
	m_buflen = c_sizeofBaseString;
}

//------------------------------------------------------------------------------
void Cstr::reserve( unsigned int size )
{
	if ( size < m_buflen )
	{
		return;
	}

	if ( m_str != m_smallbuf )
	{
		delete[] m_str;
	}
	
	m_str = new char[size + 1];
	m_buflen = size;
	m_len = 0;
}

//-----------------------------------------------------------------------------
Cstr::Cstr( wchar_t* wcs )
{
	m_len = 0;
	m_smallbuf[0] = 0;
	m_str = m_smallbuf;
	m_buflen = c_sizeofBaseString;

	toChar( wcs );
}

//-----------------------------------------------------------------------------
Cstr::Cstr( unsigned int len )
{
	m_len = 0;
	
	if ( len < c_sizeofBaseString )
	{
		m_str = m_smallbuf;
		m_buflen = c_sizeofBaseString;
	}
	else
	{
		m_str = new char[ len + 1 ];
		m_buflen = len;
	}
}

//-----------------------------------------------------------------------------
void Cstr::giveOwnership( char *dynamicMemory, unsigned int len /*=0*/ )
{
	clear();
	
	if ( !dynamicMemory )
	{
		return;
	}
	
	len = len ? len : (unsigned int)strlen( dynamicMemory );

	if ( len < c_sizeofBaseString )
	{
		memcpy( m_str, dynamicMemory, len );
		delete[] dynamicMemory;
		m_str[ len ] = 0;
		m_buflen = c_sizeofBaseString;
	}
	else
	{
		m_str = dynamicMemory;
		m_buflen = len;
	}

	m_len = len;
}

//-----------------------------------------------------------------------------
char* Cstr::getOwnership( unsigned int* len /*=0*/ )
{
	if ( !m_len )
	{
		if ( len )
		{
			*len = 0;
		}
		return 0;
	}

	char *ret;
	if ( m_str == m_smallbuf )
	{
		ret = new char[ m_len ];
		memcpy( ret, m_smallbuf, m_len );
	}
	else
	{
		ret = m_str;
		m_str = m_smallbuf;
	}

	if ( len )
	{
		*len = m_len;
	}

	m_len = 0;
	m_buflen = c_sizeofBaseString;

	return ret;
}

//-----------------------------------------------------------------------------
void Cstr::setString( const char* buf, unsigned int len /*=0*/ ) 
{
	if ( buf == 0 ) // equate to a null? accept as a clear string
	{
		clear();
		return;
	}
	
	len = len ? len : (unsigned int)strlen( buf );

	if ( len >= m_buflen )
	{
		m_buflen = len;

		if ( m_str != m_smallbuf )
		{
			delete[] m_str;
		}

		m_str = new char[ m_buflen + 1 ];
	}

	memcpy( m_str, buf, len );
	m_str[len] = 0;
	m_len = len;
}

//-----------------------------------------------------------------------------
bool Cstr::fileToBuffer( const char* fileName )
{
	if ( !fileName )
	{
		setString( 0, 0 );
		return false;
	}

#ifdef _WIN32
	struct _stat sbuf;
	int ret = _stat( fileName, &sbuf );
#else
	struct stat sbuf;
	int ret = stat( fileName, &sbuf );
#endif

	if ( ret != 0 )
	{
		setString( 0, 0 );
		return false;
	}

	FILE *infil = fopen( fileName, "rb" );

	if ( !infil )
	{
		setString( 0, 0 );
		return false;
	}

	char *inbuf = new char[ sbuf.st_size + 1 ];
	inbuf[sbuf.st_size] = 0;

	fread( inbuf, sbuf.st_size, 1, infil );

	if ( !ferror(infil) )
	{
		fclose( infil );
		setString( inbuf, sbuf.st_size );
		delete[] inbuf;
		return true;
	}
	else
	{
		fclose( infil );
		delete[] inbuf;
		return false;
	}
}

//-----------------------------------------------------------------------------
bool Cstr::bufferToFile( const char* fileName, bool append /*=false*/ ) const
{
	if ( !fileName )
	{
		return false;
	}
	
	FILE *outfil = append ? fopen( fileName, "a+b" ) : fopen( fileName, "wb" );
	if ( !outfil )
	{
		return false;
	}

	int ret = (int)fwrite( c_str(), m_len, 1, outfil );
	fclose( outfil );

	return ret == 1;
}

//-----------------------------------------------------------------------------
wchar_t* Cstr::toWide( wchar_t *dest )
{
	size_t mbslen = mbstowcs( 0, m_str, 0 );
	if ( mbslen < 0 )
	{
		return 0;
	}

	mbstowcs( dest, m_str, mbslen );
	dest[mbslen] = 0;
	return dest;
}

//-----------------------------------------------------------------------------
char* Cstr::toChar( const wchar_t* wcs )
{
	const wchar_t* convert = wcs ? wcs : m_wstr;

	size_t wcslen = wcstombs( NULL, convert, 0 );

	if ( wcslen < 0 )
	{
		return 0;
	}

	wcslen++;

	char *dest = new char[ wcslen ];

	size_t newLen = wcstombs( dest, convert, wcslen );

	if ( (int)newLen == -1 ) // no go
	{
		delete[] dest;
		return 0;
	}

	dest[newLen] = 0;

	giveOwnership( dest, (unsigned int)newLen );

	return m_str;
}

//-----------------------------------------------------------------------------
bool Cstr::replace( const char* oldstr, const char* newstr, bool ignoreCase /*=false*/ )
{
	if ( !oldstr || !newstr )
	{
		return false;
	}

	int offset = find( oldstr, ignoreCase );
	if ( offset == -1 )
	{
		return false;
	}
	char *occurance = m_str + offset;

	unsigned int oldlen = (unsigned int)strlen( oldstr );
	unsigned int newlen = (unsigned int)strlen( newstr );

	if ( oldlen == newlen )
	{
		memcpy( occurance, newstr, newlen );
	}
	else if ( newlen < oldlen )
	{
		memcpy( occurance, newstr, newlen );
		memcpy( occurance + newlen, occurance + oldlen, ((m_len - (occurance - c_str())) - oldlen) + 1 );
		m_len -= oldlen - newlen;
		m_str[m_len] = 0;
	}
	else
	{
		Cstr tail = occurance + oldlen;

		m_len = (int)(occurance - m_str);
		
		append( newstr );
		append( tail );
	}

	return true;
}

//-----------------------------------------------------------------------------
// remove the whitespace from the begining and end (and ONLY the begining and end)
// of a string.
void Cstr::trim()
{
	// this can be more efficient, if this becomes part of a tight loop
	// consider optimizing
			
	if ( !m_len )
	{
		return;
	}

	unsigned int start;
	unsigned int end;

	// mark leading chunk of whitespace
	for( start=0; start<m_len && isspace( *(m_str + start) ) ; start++ );

	// mark trailing chunk of whitespace
	for( end=m_len; end>start && isspace( *(m_str + end - 1) ) ; end-- );

	m_len = end - start;

	if ( end == start )
	{
		clear();
	}
	else if ( start )
	{
		memcpy( m_str, m_str + start, m_len );
	}

	m_str[m_len] = 0;
}

//-----------------------------------------------------------------------------
void Cstr::truncate( unsigned int newLen, bool recoverMemory /*=false*/ )
{
	if ( newLen > m_len )
	{
		return;
	}

	m_str[ newLen ] = 0;
	m_len = newLen;

	if ( recoverMemory && (m_len >= c_sizeofBaseString) )
	{
		Cstr newStr( m_str, m_len );

		m_str = newStr.m_str; // take the memory
		m_buflen = newStr.m_buflen;
		
		newStr.m_str = newStr.m_smallbuf; // supress the delete when newStr goes out of scope
	}
}

//-----------------------------------------------------------------------------
unsigned int Cstr::dequeue( const unsigned int bytes, Cstr *str /*=0*/ )
{
	unsigned int ret = 0;
	if ( bytes == 0 )
	{
		str->clear();
		ret = 0;
	}
	else if ( bytes > m_len )
	{
		if ( str )
		{
			str->setString( c_str(), m_len );
		}

		ret = m_len;
		clear();
	}
	else
	{
		if ( str )
		{
			str->setString( c_str(), bytes );
		}
		
		unsigned int newLen = m_len - bytes;
		if ( newLen < c_sizeofBaseString )
		{
			if ( m_len >= c_sizeofBaseString )
			{
				memcpy( m_smallbuf, m_str + bytes, newLen );
				m_smallbuf[newLen] = 0;
				delete[] m_str;
				m_str = m_smallbuf;
				m_buflen = c_sizeofBaseString;
			}
			else
			{
				memcpy( m_smallbuf, m_smallbuf + bytes, newLen );
				m_smallbuf[newLen] = 0;
			}
		}
		else
		{
			memcpy( m_str, m_str + bytes, newLen );
			m_str[newLen] = 0;
		}

		m_len = newLen;
		ret = bytes;
	}

	return ret;
}

//-----------------------------------------------------------------------------
bool Cstr::isMatch( const char* buf, bool matchCase /*=true*/, bool matchPartial /*=false*/ ) const
{
	if( !buf || !m_len )
	{
		return false;
	}

	unsigned int len = (unsigned int)strlen( buf );
	if ( !matchPartial && len != m_len )
	{
		return false;
	}

	if ( matchCase )
	{
		return strncmp( m_str, buf, len ) == 0;
	}
	else
	{
#ifdef _WIN32
		return _strnicmp( m_str, buf, len ) == 0;
#else
		return strncasecmp( m_str, buf, len ) == 0;
#endif
	}
}

//-----------------------------------------------------------------------------
bool Cstr::isWildMatch( const char* pattern, const bool matchCase /*=true*/ ) const
{
	if ( !pattern )
	{
		return false;
	}

	if ( pattern[0] == 0 )
	{
		return m_str[0] == 0;
	}

	bool match = true;
	const char* after = 0;
	const char* str = m_str;
	char t;
	char w;

	for(;;)
	{
		t = *str;
		w = *pattern;
		if ( !t )
		{
			if ( !w )
			{
				break; // "x" matches "x"
			}
			else if (w == '*')
			{
				pattern++;
				continue; // "x*" matches "x" or "xy"
			}
			match = false;
			break; // "x" doesn't match "xy"
		}
		else
		{
			if ( matchCase ? t != w : tolower(t) != tolower(w) )
			{
				if (w == '*')
				{
					after = ++pattern;
					continue; // "*y" matches "xy"
				}
				else if (after)
				{
					pattern = after;
					w = *pattern;
					if ( !w )
					{
						break; // "*" matches "x"
					}
					else if (t == w)
					{
						pattern++;
					}
					str++;
					continue; // "*sip*" matches "mississippi"
				}
				else
				{
					match = false;
					break; // "x" doesn't match "y"
				}
			}
		}
		str++;
		pattern++;
	}

	return match;
}

//-----------------------------------------------------------------------------
int Cstr::find( const char* buf, bool matchCase ) const
{
	if ( !buf || !m_len )
	{
		return -1;
	}

	const char* found = 0;
	if ( matchCase )
	{
		found = strstr( m_str, buf );
	}
	else
	{
#ifdef _WIN32
		found = strstr( m_str, buf );//_strcasestr( m_str, buf );
#else
		found = strcasestr( m_str, buf );
#endif
	}

	return found ? (int)(found - m_str) : -1;
}

//-----------------------------------------------------------------------------
const char* Cstr::format( const char* format, ... )
{
	va_list arg;
	va_start( arg, format );
	Cstr::format( format, arg );
	va_end( arg );
	return c_str();
}

//-----------------------------------------------------------------------------
const char* Cstr::format( const char* format, va_list arg )
{
	char buf[ 8000 ];
	int len = vsprintf( buf, format, arg );
	setString( buf, len );
	return c_str();
}

//-----------------------------------------------------------------------------
static inline unsigned int getNextStringSize( unsigned int sizeNeeded )
{
	unsigned int size;

	for( size = c_sizeofBaseString * 2; size < sizeNeeded ; size *=2 ) // grow by doubling
	{
		if ( size > 1000000 ) // over a meg? grow by 500K increments
		{
			for( size = c_sizeofBaseString * 2; size < sizeNeeded ; size += 500000 );
			return size + 1;
		}
	}

	return size + 1;
}

//-----------------------------------------------------------------------------
const char* Cstr::append( const void* vbuf, unsigned int len )
{
	if ( !vbuf )
	{
		return 0;
	}

	const char* buf = (const char *)vbuf;

	len = len ? len : (unsigned int)strlen( buf );

	unsigned int newLen = len + m_len;

	if ( newLen < m_buflen ) // trivial case
	{
		memcpy( m_str + m_len, buf, len );
		m_len = newLen;
		m_str[m_len] = 0;
		return m_str;
	}
	else
	{
		m_buflen = getNextStringSize( newLen );
		char *T = new char[ m_buflen + 1 ];
		
		memcpy( T, m_str, m_len );
		memcpy( T + m_len, buf, len );
		T[ newLen ] = 0;

		if ( m_str != m_smallbuf )
		{
			delete[] m_str;
		}
		m_str = T;
		m_len = newLen;
		
		return m_str;
	}
}

//-----------------------------------------------------------------------------
const char* Cstr::append( const char c )
{
	m_str[ m_len++ ] = c;
	if ( m_len >= m_buflen )
	{
		m_buflen = getNextStringSize( m_len );
		char *T = new char[ m_buflen + 1 ];
		memcpy( T, m_str, m_len );

		if ( m_str != m_smallbuf )
		{
			delete[] m_str;
		}

		m_str = T;
	}

	m_str[ m_len ] = 0;
	return m_str;
}

//-----------------------------------------------------------------------------
const char* Cstr::appendFormat( const char* format, ... )
{
	va_list arg;
	va_start( arg, format );
	appendFormat( format, arg );
	va_end( arg );
	return c_str();
}

//-----------------------------------------------------------------------------
const char* Cstr::appendFormat( const char* format, va_list arg )
{
	char buf[8000];
	int len = vsprintf( buf, format, arg );
	append( buf, (unsigned int)len );
	return c_str();
}

//------------------------------------------------------------------------------
bool Cstr::getToken( unsigned int tokenNum, Cstr& token )
{
	const char *buf = c_str();

	unsigned int i = 0;
	
	for( unsigned int j=0; j<=tokenNum ; j++ )
	{
		// eat leading whitespace
		for( ; i<m_len && isspace(buf[i]) ; i++ );

		if ( i >= m_len )
		{
			return false;
		}

		if ( j == tokenNum )
		{
			token.clear();
		
			for( ; i<m_len && !isspace(buf[i]) ; i++ )
			{
				token.append( buf[i] );
			}
		}
		else
		{
			for( ; i<m_len && !isspace(buf[i]) ; i++ );
		}
	}

	return true;
}

#endif

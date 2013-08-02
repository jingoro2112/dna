#ifndef SIMPLELOG_HPP
#define SIMPLELOG_HPP
/*------------------------------------------------------------------------------*/

#include <time.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#ifdef _WIN32
#pragma warning (disable : 4996) // remove Windows nagging about using _strincmp() etc
#else
#include <unistd.h>
#include <sys/time.h>
static timespec s_ts;
#endif

//------------------------------------------------------------------------------
inline void simpleOut( const char* text )
{
	printf( "%s\n", text );
}

//------------------------------------------------------------------------------
// ultra-simple lightweight logging facility to get directly at low-level spammy  log messages.
class SimpleLog
{
public:

	void setFile( const char *path ) { strcpy(m_filename, path); }
	void setCallback( void (*callback)(const char* msg) ) { m_callback = callback; }
	
	void operator()( const char* format, ... )
	{
		va_list arg;
		va_start( arg, format );
		outEx( format, arg );
		va_end ( arg );
	}

	void out( const char* format, ... )
	{
		va_list arg;
		va_start( arg, format );
		outEx( format, arg );
		va_end ( arg );
	}

	SimpleLog() { m_filename[0] = 0; m_callback = simpleOut; }

	
private:

	void outEx( const char* format, va_list arg )
	{
		if ( !m_callback && !m_filename )
		{
			return;
		}
		
		struct tm tmbuf;
		time_t tod = time(0);
		memcpy( &tmbuf, localtime(&tod), sizeof(struct tm) );
		char temp[8192];

#ifdef _WIN32
     		int tick = GetTickCount();
#else   
        	clock_gettime( CLOCK_REALTIME, &s_ts );
        	int tick = ( s_ts.tv_sec * 1000 ) + ( s_ts.tv_nsec / 1000000 );
#endif

		sprintf( temp, "%02d %02d:%02d:%02d.%03d> ",
				 tmbuf.tm_mday,
				 tmbuf.tm_hour,
				 tmbuf.tm_min,
				 tmbuf.tm_sec,
				 tick % 1000 );

		int len = vsprintf( temp + 17, format, arg ) + 17;

		if ( temp[len - 1] == '\n' )
		{
			len--;
			temp[len] = 0;
		}

		if ( m_callback )
		{
			m_callback( temp );
		}

		if ( m_filename[0] )
		{
			FILE *fil = fopen( m_filename, "a" );
			if ( fil )
			{
				temp[len++] = '\n';
				temp[len] = 0;

				fwrite( temp, len, 1, fil );
				fclose( fil );
			}
		}
	}

	void (*m_callback)(const char* msg);
	char m_filename[256];
};

#endif

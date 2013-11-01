/* Copyright: (c) 2013 by Curt Hartung avr@northarc.com
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */
#ifndef ARCHITECTURE_H
#define ARCHITECTURE_H

#if defined(_WIN32)

 #include <conio.h>
 #include <winsock2.h>
 #include <windows.h>
 #include <process.h>

#elif defined(__CYGWIN__)

 #include <time.h>
 #include <unistd.h>
 #include <sys/time.h>
 #include <sys/types.h>
 #define InterlockedIncrement(V) __sync_add_and_fetch((V), 1)
 #define InterlockedDecrement(V) __sync_sub_and_fetch((V), 1)

#else

 #ifdef __MACH__
  #include <mach/clock.h>
  #include <mach/mach.h>
 #endif

 #include <time.h>
 #include <unistd.h>
 #include <sys/time.h>
 #include <sys/types.h>
 #include <sys/syscall.h>
 #define InterlockedIncrement(V) __sync_add_and_fetch((V), 1)
 #define InterlockedDecrement(V) __sync_sub_and_fetch((V), 1)
#endif

#include <stdio.h>

#include "str.hpp"

// architecture-specific glue
namespace Arch
{

inline int kbhit()
{
#if defined(_WIN32)
	return ::kbhit();
#else
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	
	fd_set rdfs;
	FD_ZERO( &rdfs );
	FD_SET(STDIN_FILENO, &rdfs );
	select( STDIN_FILENO+1, &rdfs, 0, 0, &tv );
	
	return FD_ISSET( STDIN_FILENO, &rdfs );
#endif
}

//------------------------------------------------------------------------------
inline void sleep( unsigned int milliseconds )
{
#if defined(_WIN32)
	::Sleep( milliseconds );
#else
	usleep( milliseconds * 1000 );
#endif
}

//------------------------------------------------------------------------------
inline int getpid()
{
#if defined(_WIN32)
	return (int)_getpid();
#else
	return (int)::getpid();
#endif
}

//------------------------------------------------------------------------------
inline int gettid()
{
#if defined(_WIN32)
	return (int)GetCurrentThreadId();
#elif defined(__CYGWIN__)
	return (int)-1;
#else
	return (int)syscall(SYS_gettid);
#endif
}

//------------------------------------------------------------------------------
inline const char* asciiDump( const void* v, unsigned int len, Cstr* str =0 )
{
	char* data = (char *)v;
	Cstr local;
	Cstr *use = str ? str : &local;
	use->clear();
	for( unsigned int i=0; i<len; i++ )
	{
		use->appendFormat( "0x%08X: ", i );
		char dump[18];
		unsigned int j;
		for( j=0; j<16 && i<len; j++, i++ )
		{
			dump[j] = isgraph((unsigned char)data[i]) ? data[i] : '.';
			dump[j+1] = 0;
			use->appendFormat( "%02X ", (unsigned char)data[i] );
		}

		for( ; j<16; j++ )
		{
			use->appendFormat( "   " );
		}
		i--;
		use->appendFormat( ": " );
		use->append( dump );
		use->append( "\n" );
	}

	if ( !str )
	{
		printf( "%s\n", use->c_str() );
	}
	
	return str ? str->c_str() : 0;
}

// on windows sytems, the MillisecondTick function is around 10 times
// faster (measured) so use it when you don't need the precision.
//------------------------------------------------------------------------------
inline long long millisecondTick()
{
#ifdef _WIN32
	FILETIME ft;
	GetSystemTimeAsFileTime( &ft );
	ULARGE_INTEGER ftui;
	ftui.LowPart = ft.dwLowDateTime;
	ftui.HighPart = ft.dwHighDateTime;
	return ftui.QuadPart / 10000;
#else

	struct timespec ts;

#ifdef __MACH__ 
	clock_serv_t cclock;
	mach_timespec_t mts;
	host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
	clock_get_time(cclock, &mts);
	mach_port_deallocate(mach_task_self(), cclock);
	ts.tv_sec = mts.tv_sec;
	ts.tv_nsec = mts.tv_nsec;
#else
	clock_gettime( CLOCK_REALTIME, &ts );
#endif
	
	return (long long)(ts.tv_nsec/1000000) + (ts.tv_sec * 1000);
#endif
}

//------------------------------------------------------------------------------
inline long long nanosecondTick()
{
#ifdef _WIN32
	LARGE_INTEGER freq, out;
	QueryPerformanceFrequency( &freq );
	QueryPerformanceCounter( &out );
	return (out.QuadPart * 1000000000) / freq.QuadPart;
#else
	
	struct timespec ts;

#ifdef __MACH__ // OS X does not have clock_gettime, use clock_get_time
	clock_serv_t cclock;
	mach_timespec_t mts;
	host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
	clock_get_time(cclock, &mts);
	mach_port_deallocate(mach_task_self(), cclock);
	ts.tv_sec = mts.tv_sec;
	ts.tv_nsec = mts.tv_nsec;
#else
	clock_gettime( CLOCK_REALTIME, &ts );
#endif

	return (long long)ts.tv_nsec + ((long long)ts.tv_sec * 1000000000);
#endif
}

}

#endif


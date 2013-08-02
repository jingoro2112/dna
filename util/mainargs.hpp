#ifndef MAINARGS_HPP
#define MAINARGS_HPP
//------------------------------------------------------------------------------

#ifdef _WIN32
#pragma warning (disable : 4996) // remove Windows nagging about using _strincmp() etc
#endif

#include <string.h>
#include <ctype.h>
#include <stdlib.h>

const int c_maxArguments = 256;

//------------------------------------------------------------------------------
class MainArgs
{
public:
	MainArgs( int argn, char *argv[] ) { m_argn = argn;	m_argv = (char **)argv; m_owned = false; }
	MainArgs( const char* argstr ) { initFromString( argstr ); }
	inline MainArgs( const wchar_t* argstr );
	inline ~MainArgs();

	// if "atomic" is FALSE then an option can be a substring,
	// so -tvset would return true for "set"
	inline bool isSet( const char *opt, bool atomic =true );

	// look for the number accompanying this option.
	inline bool isNumSet( const char *opt, int *num ) { return isNumSet(opt, (unsigned int *)num); }
	inline bool isNumSet( const char *opt, unsigned int *num );

	// look for the string accompanying this option.
	inline bool isStringSet( const char *opt, char *string, int max =256 );

private:

	inline void initFromString( const char* argstr );
	
	bool m_owned;
	int m_argn;
	char **m_argv;
};

//------------------------------------------------------------------------------
MainArgs::MainArgs( const wchar_t* argstr )
{
	size_t wcslen = wcstombs( NULL, argstr, 0 );
	if ( wcslen < 0 )
	{
		m_owned = false;
		m_argv = 0;
		m_argn = 0;
		return;
	}

	wcslen++;

	char *dest = new char[ wcslen ];

	size_t len = wcstombs( dest, argstr, wcslen );

	if ( (int)len == -1 ) // no go
	{
		delete[] dest;
		m_owned = false;
		m_argv = 0;
		m_argn = 0;
		return;
	}

	dest[len] = 0;

	initFromString( dest );

	delete[] dest;
}

//------------------------------------------------------------------------------
MainArgs::~MainArgs()
{
	if ( m_owned )
	{
		for( int i=0; i<m_argn; i++ )
		{
			delete[] m_argv[i];
		}
		delete[] m_argv;
	}
}

//------------------------------------------------------------------------------
bool MainArgs::isSet( const char *opt, bool atomic /*=true*/ )
{
	if ( !opt )
	{
		return false;
	}

	for( int i=1; i<m_argn ; i++ )
	{
		if ( !strcmp(opt, m_argv[i]) )
		{
			return true;
		}

		if ( !atomic )
		{
			if ( strstr(opt, m_argv[i]) )
			{
				return true;
			}
		}
	}

	return false;
}

//------------------------------------------------------------------------------
bool MainArgs::isNumSet( const char *opt, unsigned int *num )
{
	if ( !opt || !num )
	{
		return false;
	}

	for( int i=1; i<m_argn-1 ; i++ )
	{
		if ( !strcmp(opt, m_argv[i]) )
		{
			i += 1;
			if ( i >= m_argn ) // is there another argument?
			{
				return false;
			}

			unsigned int len = (unsigned int)strlen( m_argv[i] );

			if ( !isdigit( *(m_argv[i]) ) // is it a number?
				 && (len == 1
					 && *(m_argv[i]) != '-'
					 && *(m_argv[i]) != '+' ) )
			{
				return false;
			}

			for( unsigned int j=1; j<len ; j++ )
			{
				if ( !isdigit( *(m_argv[i] + j) ) )
					return false;
			}

			*num = (unsigned int)atoi( m_argv[i] );

			return true;
		}
	}

	return false;
}

//------------------------------------------------------------------------------
bool MainArgs::isStringSet( const char *opt, char *string, int max /*=256*/ )
{
	if ( !opt || !string )
	{
		return false;
	}

	for( int i=1; i<m_argn-1 ; i++ )
	{
		if ( !strcmp(opt, m_argv[i]) )
		{
			i += 1;
			if ( i >= m_argn ) // is there another argument?
				return false;

			if ( (int)strlen(m_argv[i]) > max )
			{
				strncpy( string, m_argv[i], max-1 );
				string[max-1] = 0;
			}
			else
			{
				strcpy( string, m_argv[i] );
			}

			return true;
		}
	}

	return false;
}

//------------------------------------------------------------------------------
void MainArgs::initFromString( const char* argstr )
{
	m_owned = true;
	m_argv = new char*[c_maxArguments+1];
	char argTemp[1024];
	int pos = 0;
	m_argv[0] = new char[ 11 ];
	sprintf( m_argv[0], "<PROGNAME>" );
	for( m_argn=1;  argstr[pos]; m_argn++ )
	{
		int i = 0;

		for( ; argstr[pos] && isspace(argstr[pos]); pos++ ); // strip leading white

		for( ; argstr[pos] && !isspace(argstr[pos]) && i<1024; i++, pos++ )
		{
			if ( argstr[pos] == '\"' )
			{
				pos++; // skip quote
				for( ; argstr[pos] && argstr[pos] != '\"' && i<1024; i++, pos++ )
				{
					argTemp[i] = argstr[pos];
				}
				break;
			}
			else
			{
				argTemp[i] = argstr[pos];
			}
		}
		if ( argstr[pos] )
		{
			pos++;
		}
		argTemp[i] = 0;

		m_argv[m_argn] = new char[ strlen(argTemp) ];
		strcpy( m_argv[m_argn], argTemp );
	}
}

#endif

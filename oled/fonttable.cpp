#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

#include "font.hpp"
#include "../util/str.hpp"

bool parseInfile( Cstr const& in, Cstr &out );

//------------------------------------------------------------------------------
int usage()
{
	printf( "Usage: fonttable <infile> <outfile>\n" );
	return -1;
}

//------------------------------------------------------------------------------
int main( int argn, char *argv[] )
{
	if ( argn != 3 )
	{
		return usage();
	}

	Cstr out;
	if ( !parseInfile(Cstr(argv[1]), out) )
	{
		return usage();
	}

	if ( !out.bufferToFile( argv[2] ) )
	{
		printf( "Could not save [%s]\n", argv[2] );
		return usage();
	}

	return 0;
}

//------------------------------------------------------------------------------
bool parseInfile( Cstr const& in, Cstr& out )
{
	out.clear();

	Font font;

	Cstr fontNameUpper;
	Cstr fontName;
	for( unsigned int i=0; i<in.size(); i++ )
	{
		if ( in[i] == '.' )
		{
			fontNameUpper += "_";
			fontName += "_";
		}
		else
		{
			fontNameUpper += toupper(in[i]);
			fontName +=in[i];
		}
	}

	out.format(
			   "#ifndef FONT_%s_H\n"
			   "#define FONT_%s_H\n\n"
			   "/*------------------------------------------------------------------------------*\n"
			   "* Copyright: (c) 2013 by Curt Hartung avr@northarc.com\n"
			   "* This work is released under the Creating Commons 3.0 license\n"
			   "* found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode\n"
			   "* and in the LICENCE.txt file included with this distribution\n"
			   "*/\n"
			   "\n"
			   "******************************************************************************\n"
			   "******************************************************************************\n"
			   "********* THIS FILE IS AUTOMATICALLY GENERATED, DO NOT EDIT DIRECTLY *********\n"
			   "******************************************************************************\n"
			   "******************************************************************************\n"
			   "\n", fontNameUpper.c_str(), fontNameUpper.c_str() );

	char temp[256];
	
	unsigned int currentDataBlockPointer = 0;
	Cstr data;

	if ( !font.load(in) )
	{
		printf( "unable to load font [%s], aborting\n", temp+1 );
		return false;
	}
	else
	{
		printf( "loaded font [%s]\n", in.c_str() );
	}

	out.appendFormat( "const PROGMEM unsigned int font_%s[]=\n{\n\t", fontName.c_str() );
	
	for( unsigned char g = 0x20; g<0x7E; g++ )
	{
		Font::Metrics const& metrics = font.getMetrics()[ (unsigned char)font.glyphFromChar(g) ];

		int w,h;
		const unsigned char *bmp = font.getBitmap( w, h );

		out.appendFormat( "0x%04X, // \"%c\"\n\t", currentDataBlockPointer, g );

		data.appendFormat( "%2d, %2d, %2d, %2d, ", metrics.w, metrics.h, metrics.pre, metrics.post );
		currentDataBlockPointer += 4;

		unsigned char byte;
		unsigned char accum;

		for( int r = metrics.y; r < (metrics.y + metrics.h); r++ )
		{
			byte = 0;
			accum = 0;

			for( int c = metrics.x; c < metrics.x + metrics.w; c++ )
			{
				if ( bmp[ (r * w) + c ] )
				{
					byte |= (1 << (7 - accum));
				}

				if (++accum == 8)
				{
					byte = 0;
					accum = 0;

					data.appendFormat( "0x%02X, ", byte );
					currentDataBlockPointer++;
				}
			}

			if ( accum )
			{
				data.appendFormat( "0x%02X, ", byte );
				currentDataBlockPointer++;
			}
		}

		data.appendFormat( "\t// %c\n\t", g );
	}

	out.append( "\n};\n\n" );

	data.append( "\n};\n\n" );
	out.appendFormat("\n\n//------------------------------------------------------------------------------\n"
					 "const PROGMEM unsigned char dataBlock[%d]=\n{\n\t", currentDataBlockPointer );
	out.append( data );
	out.append( "\n#endif\n" );

	return true;
}
 

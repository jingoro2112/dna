#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

#include "font.hpp"
#include "../util/str.hpp"

bool parseInfile( Cstr const& in, Cstr &out );

//------------------------------------------------------------------------------
int usage()
{
	printf( "Usage: ofont <infile> <outfile>\n" );
	return -1;
}

//------------------------------------------------------------------------------
int main( int argn, char *argv[] )
{
	if ( argn != 3 )
	{
		return usage();
	}

	Cstr infile;
	if ( !infile.fileToBuffer(argv[1]) )
	{
		printf( "Could not open [%s]\n", argv[1] );
		return usage();
	}
	
	Cstr out;
	if ( !parseInfile(infile, out) )
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

	unsigned int pos = 0;
	const char* str = in.c_str();
	Font font;

	out = "/*------------------------------------------------------------------------------*/\n"
		  "/********************************************************************************/\n"
		  "/********************************************************************************/\n"
		  "//                   AUTOMATICALLY GENERATED. DO NOT MODIFY\n"
		  "/********************************************************************************/\n"
		  "/********************************************************************************/\n"
		  "/*------------------------------------------------------------------------------*/\n"
		  "#ifndef _STRINGLIST_H\n"
		  "#define _STRINGLIST_H\n\n";


	unsigned int i;
	char temp[256];
	
	unsigned int currentDataBlockPointer = 0;
	Cstr data;

	while( pos < in.size() )
	{
		for( ;pos<in.size() && isspace(str[pos]); pos++); // skip leading whitespace

		i = 0;
		for( ;pos<in.size() && str[pos] != '\n' ; pos++ )
		{
			temp[i++] = str[pos];
		}
		temp[i] = 0;

		switch( temp[0] )
		{
			case '~':
			{
				pos++;
				if ( !font.load(temp + 1) )
				{
					printf( "unable to load font [%s], aborting\n", temp+1 );
					return false;
				}
				else
				{
					printf( "loaded font [%s]\n", temp+1 );
				}

				out.appendFormat( "const PROGMEM unsigned int font_%s[]=\n{\n\t", temp + 1 );

				for( unsigned char g = 0x20; g<0x7E; g++ )
				{
					Font::Metrics const& metrics = font.getMetrics()[ (unsigned char)font.glyphFromChar(g) ];

					int w,h;
					const unsigned char *bmp = font.getBitmap( w, h );

					out.appendFormat( "0x%04X, // \"%c\"\n\t", currentDataBlockPointer, g );

					data.appendFormat( "0x%02X, 0x%02X, 0x%02X, %d, ", metrics.w, metrics.h, metrics.pre, metrics.post );
					currentDataBlockPointer += 4;

					unsigned char byte;
					unsigned char accum;

					printf( "%d\n", metrics.h * metrics.w );
					for( int r = metrics.y; r < metrics.y + metrics.h; r++ )
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
			}
			
			case '|':
			{
				// todo- load raw bitmap image
				break;
			}
		}
	}

	data.append( "\n};\n\n" );
	out.appendFormat("\n\n//------------------------------------------------------------------------------\n"
					 "const PROGMEM unsigned char dataBlock[%d]=\n{\n\t", currentDataBlockPointer );
	out.append( data );
	out.append( "\n#endif\n" );

	return true;
}
 


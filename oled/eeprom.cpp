#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

#include "eeprom_types.h"

#include "../util/font.hpp"
#include "../util/str.hpp"
#include "../util/linkhash.hpp"
#include "../util/architecture.hpp"
#include "../util/hash.hpp"
#include "../firmware/dna/dna_types.h"
#include "../firmware/morlock/morlock_defs.h"

bool parseInfile( Cstr const& in, Cstr &out );

//------------------------------------------------------------------------------
int usage()
{
	printf( "Usage: eeprom <font1> <font2> <...>\n" );
	return -1;
}

//------------------------------------------------------------------------------
struct FontTable
{
	FontCharEntry entry[16];
};

//------------------------------------------------------------------------------
uint16 hashString( const char* string )
{
	return (uint16)Hash::hashStr( string );
}

//------------------------------------------------------------------------------
bool parseString( Cstr& in, unsigned int* pos, Cstr& string )
{
//	printf( "parsing [%d][%s]\n", *pos, in.c_str() + *pos );
	
	if ( *pos >= in.size() )
	{
		return false;
	}
	
	while( isspace(in[*pos]) && (*pos < in.size()) )
	{
		(*pos)++;
	}
	
	if ( *pos >= in.size() )
	{
		return false;
	}

	bool quoted = false;
	if ( in[*pos] == '\"' )
	{
		quoted = true;
		(*pos)++;
	}

	string.clear();

	if ( quoted )
	{
		while( (*pos < in.size()) && (in[*pos] != '\"') )
		{
			string += in[(*pos)++];
		}
		
		if ( *pos >= in.size() || in[*pos] != '\"' )
		{
			return false;
		}
	}
	else
	{
		while( (*pos < in.size()) && !isspace(in[*pos]) )
		{
			string += in[(*pos)++];
		}

		if ( *pos >= in.size() )
		{
			return false;
		}
	}

	(*pos)++;

	return true;
}

CLinkHash<Cstr> stringList;
unsigned int stringNum = 0;

//------------------------------------------------------------------------------
int getStringOffset( const char* string )
{
	for( Cstr* S = stringList.getFirst(); S; S = stringList.getNext() )
	{
		if ( *S == string )
		{
			return stringList.getCurrentKey();
		}
	}

	return 0;
}

//------------------------------------------------------------------------------
int addToStringTable( Cstr& in, Cstr& binout, Cstr& defines, Cstr& out )
{
	int currentDataBlockPointer = binout.size();
	int lastOffset = 0;
	
	Cstr temp;
	unsigned int pos = 0;

	while( pos < in.size() )
	{
		temp.clear();
		for( ;in[pos] && pos < in.size(); pos++ )
		{
			if ( in[pos] == '\n' || in[pos] == '\r' )
			{
				break;
			}

			temp += in[pos];
		}

		lastOffset = getStringOffset(temp);
		if ( lastOffset )
		{
			continue;
		}
		
		binout.append( temp );
		char n = 0;
		binout.append( &n, 1 ); // terminate in image

		for( pos++; (pos < in.size()) && isspace(in[pos]); pos++ );

		if ( temp.size() > 32 )
		{
			printf( "WARNING: stringlen[%d]:[%s] is too long\n", temp.size(), temp.c_str() );
		}

		defines.appendFormat( "#define S_%03d %d // \"%s\"\n", stringNum++, currentDataBlockPointer, temp.c_str() );
		out.appendFormat( "\t\"%s\",\n", temp.c_str() );

		stringList.addItem( temp, currentDataBlockPointer );
		lastOffset = currentDataBlockPointer;

		currentDataBlockPointer = binout.size();
	}

	return lastOffset;
}

//------------------------------------------------------------------------------
int main( int argn, char *argv[] )
{
	Cstr out;
	Cstr binout;
	FontTable ftable[256];
	Font font[16];
	Cstr dataTable;
	
	int currentDataBlockPointer = 0;

	char numberOfFonts = argn - 1;

	// first construct the font table
	for( int i=0; i<numberOfFonts; i++ )
	{
		Cstr convertCommand;
		convertCommand.format( "convert %s.png  -background black -flatten +matte -depth 8 %s.gray", argv[i+1], argv[i+1] );
		system( convertCommand );
		
		if ( !font[i].load(argv[i+1]) )
		{
			printf( "unable to load font [%s], aborting\n", argv[i+1] );
			return false;
		}
		else
		{
			printf( "loaded font [%s]\n", argv[i+1] );
		}

		for( unsigned char g = 0x20; g<0x7E; g++ )
		{
			Font::Metrics const& metrics = font[i].getMetrics()[ (unsigned char)font[i].glyphFromChar(g) ];

			int w,h;
			const unsigned char *bmp = font[i].getBitmap( w, h );

			ftable[g].entry[i].dataOffset = currentDataBlockPointer;
			ftable[g].entry[i].w = metrics.w;
			ftable[g].entry[i].h = metrics.h;
			ftable[g].entry[i].pre = metrics.pre;
			ftable[g].entry[i].post = metrics.post;
			
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
						byte |= 1 << accum;
					}

					if ( ++accum == 8 )
					{
						dataTable.append( byte );

						byte = 0;
						accum = 0;
						currentDataBlockPointer++;
					}
				}

				if ( accum )
				{
					dataTable.append( byte );
					currentDataBlockPointer++;
				}
			}
		}
	}

	out.format( "#ifndef EEPROM_IMAGE_H\n"
				"#define EEPROM_IMAGE_H\n\n"
				"/*------------------------------------------------------------------------------*\n"
				"* Copyright: (c) 2013 by Curt Hartung avr@northarc.com\n"
				"* This work is released under the Creating Commons 3.0 license\n"
				"* found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode\n"
				"* and in the LICENCE.txt file included with this distribution\n"
				"*/\n"
				"\n"
				"//******************************************************************************\n"
				"//******************************************************************************\n"
				"//********* THIS FILE IS AUTOMATICALLY GENERATED, DO NOT EDIT DIRECTLY *********\n"
				"//******************************************************************************\n"
				"//******************************************************************************\n"
				"\n"
				"#include \"eeprom_types.h\"\n"
			  );

	for( int i=0; i<numberOfFonts; i++ )
	{
		out.appendFormat( "// font:%d name:\"%s\" height:%d\n",
						  i,
						  argv[i+1],
						  font[i].getHeight() );
	}

	int fontMetricsTableSize = (int)numberOfFonts * sizeof(FontCharEntry) * 95;

	out.append( "\n#ifndef AVR\n"
				"#define PROGMEM\n"
				"#else\n"
				"#include <avr/pgmspace.h>\n"
				"#endif\n\n" );
	
	out.appendFormat( "#define NUMBER_OF_FONTS %d\n", (int)numberOfFonts );
	out.appendFormat( "#define FONT_BLOCK_SIZE %d\n", fontMetricsTableSize );
	out.append( "\n" );
	
	out.appendFormat( "//------------------------------------------------------------------------------\n"
					  "#ifdef RESIDENT_FONT_TABLES\n"
					  "const PROGMEM unsigned char c_lookupTable[%d]=\n"
					  "{\n", fontMetricsTableSize, fontMetricsTableSize );

	binout.append( &numberOfFonts, 1 );
	
	char c;
	
	for( unsigned char g = 0x20; g<0x7F; g++ )
	{
		for( int i=0; i<numberOfFonts; i++ )
		{
			out.appendFormat( "\t0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X,\n",
							  (unsigned char)(ftable[g].entry[i].dataOffset),
							  (unsigned char)(ftable[g].entry[i].dataOffset>>8),
							  (unsigned char)ftable[g].entry[i].w,
							  (unsigned char)ftable[g].entry[i].h,
							  (unsigned char)ftable[g].entry[i].pre,
							  (unsigned char)ftable[g].entry[i].post );

			c = ftable[g].entry[i].dataOffset & 0xFF;
			binout.append( &c, 1 );
			c = ftable[g].entry[i].dataOffset >> 8;
			binout.append( &c, 1 );
			binout.append( (char*)&(ftable[g].entry[i]) + 2, sizeof(FontCharEntry) - 2 );
		}		
	}

	out.appendFormat( "};\n\n"
					  "#endif\n"
					  "\n"
					  "#define DATA_BLOCK_SIZE %d\n"
					  "\n"
					  "//------------------------------------------------------------------------------\n"
					  "#ifdef RESIDENT_FONT_TABLES\n"
					  "const PROGMEM unsigned char c_dataBlock[%d]=\n"
					  "{\n", currentDataBlockPointer, currentDataBlockPointer );
	
	for( int i=0; i<currentDataBlockPointer; i++ )
	{
		if ( !(i%16) )
		{
			out.append( "\n\t" );
		}

		out.appendFormat( "0x%02X, ", (unsigned char)dataTable[i] );
		c = (unsigned char)dataTable[i];
		binout.append( &c, 1 );
	}
	
	out.append( "\n};\n"
				"#endif\n"
				"\n" );


	Cstr in;
	if ( !in.fileToBuffer( "strings.txt") )
	{
		printf( "Could not load \"strings.txt\"\n" );
		return usage();
	}

	Cstr defines = "\n";
	out.appendFormat( "//------------------------------------------------------------------------------\n"
					  "#define STRING_TABLE_EEPROM_OFFSET %d\n"
					  "#ifdef RESIDENT_FONT_TABLES\n"
					  "const PROGMEM char *c_stringTable[]=\n{\n", currentDataBlockPointer );

	addToStringTable( in, binout, defines, out );

	if ( !in.fileToBuffer("menus.txt") )
	{
		printf( "Could not read <menus.txt>\n" );
		return usage();
	}

	Menu menu[100];
	Cstr menuTitle[100];
	memset( (char *)menu, 0, 100 * sizeof(Menu) );
	Cstr token;
	unsigned int pos = 0;
	int menus = 0;
	for( ; menus<100 && (pos < in.size()); menus++ )
	{
		// title
		if ( !parseString(in, &pos, token) )
		{
			printf( "Err<1>\n" );
			return -1;
		}

		menuTitle[menus] = token;
		
		menu[menus].head.stringTitle = addToStringTable( token, binout, defines, out );

		// parent
		if ( !parseString(in, &pos, token) )
		{
			printf( "Err<2>\n" );
			return -1;
		}

		menu[menus].head.parent = 0;

		if ( token.size() )
		{
			menu[menus].head.parent = hashString(token);
		}
		
		for( int i=0; i<MENU_ENTRIES; i++ )
		{
			if( isspace(in[pos]) )
			{
				pos++;
			}

			if ( in[pos] == '\n' )
			{
//				printf( "End of list\n" );
				break;
			}

			Cstr title;
			if ( !parseString(in, &pos, title) )
			{
//				printf( "End of menu definitions" );
				break;
			}
			menu[menus].entry[i].stringItem = addToStringTable( title, binout, defines, out );

			if ( !parseString(in, &pos, token) )
			{
				printf( "Err<4>\n" );
				return -1;
			}

			Cstr token2;
			if ( !parseString(in, &pos, token2) )
			{
				printf( "Err<6>\n" );
				return -1;
			}

			if ( token == "Entry8" )
			{
				menu[menus].entry[i].type = Entry8;

				if ( (menu[menus].entry[i].valueOffset = EEPROMConstants::offsetOf(token2)) == 0xFFFF )
				{
					printf( "EEPROM constant [%s] offset not found <1>\n", token2.c_str() );
					return -1;
				}
			}
			else if ( token == "Entry16" )
			{
				menu[menus].entry[i].type = Entry16;

				if ( (menu[menus].entry[i].valueOffset = EEPROMConstants::offsetOf(token2)) == 0xFFFF )
				{
					printf( "EEPROM constant [%s] offset not found <2>\n", token2.c_str() );
					return -1;
				}
			}
			else if ( token == "EntrySub" )
			{
				menu[menus].entry[i].type = EntrySubMenu;
				menu[menus].entry[i].valueOffset = hashString(title);
			}
			else
			{
				printf( "Err<5>\n" );
				return -1;
			}

			if ( !parseString(in, &pos, token) )
			{
				printf( "Err<9>\n" );
				return -1;
			}
			menu[menus].entry[i].min = atoi( token );

			if ( !parseString(in, &pos, token) )
			{
				printf( "Err<10>\n" );
				return -1;
			}
			menu[menus].entry[i].max = atoi( token );
		}
	}

	// string table should be fully popiulated now, add it
	out.append( "};\n"
				"#endif\n"
				"\n" );
	out += defines;

	printf( "\n" );

	currentDataBlockPointer = binout.size();
	out.appendFormat( "\n\n#define ROOT_MENU_OFFSET %d\n", currentDataBlockPointer );

	for( int m=0; m<menus; m++ )
	{
		int sub;
		// find parenst
		if ( menu[m].head.parent )
		{
			// find parent (if it exists)
			for( sub = 0; sub<menus; sub++ )
			{
//				printf( "trying [%s][%d]-[%d]\n", menuTitle[sub].c_str(), (int)hashString(menuTitle[sub]), (int)menu[m].head.parent );
				if ( hashString(menuTitle[sub]) == menu[m].head.parent )
				{
					menu[m].head.parent = sub;
					break;
				}
			}
		}

		out.appendFormat( "// %d title[%d] parent[%d]\n", m, (int)menu[m].head.stringTitle, (int)menu[m].head.parent );

		for( int e=0; e<MENU_ENTRIES; e++ )
		{
			if ( menu[m].entry[e].type == EntrySubMenu )
			{
				for( int sub = 0; sub<menus; sub++ )
				{
//					printf( "trying [%s][%d]-[%d]\n", menuTitle[sub].c_str(), (int)hashString(menuTitle[sub]), (int)menu[m].entry[e].valueOffset );

					if ( hashString(menuTitle[sub]) == menu[m].entry[e].valueOffset )
					{
						menu[m].entry[e].valueOffset = sub;
						break;
					}
				}
			}

			if ( menu[m].entry[e].stringItem )
			{
				out.appendFormat( "//   e[%d] item[%d] type[%d] valueOffset[%d] [%d/%d]\n",
								  e,
								  (int)menu[m].entry[e].stringItem,
								  (int)menu[m].entry[e].type,
								  (int)menu[m].entry[e].valueOffset,
								  (int)menu[m].entry[e].min,
								  (int)menu[m].entry[e].max );
			}
		}

		binout.append( (char *)&(menu[m]), sizeof(Menu) );
	}

//	Arch::asciiDump( binout.c_str() + currentDataBlockPointer, binout.size() - currentDataBlockPointer );
	
	out += "\n\n#endif\n";

	if ( !out.bufferToFile("eeprom_image.h") )
	{
		printf( "Could not save font_loader.h\n" );
		return usage();
	}

	if ( !binout.bufferToFile("eeprom_image.bin") )
	{
		printf( "Could not save font_loader.bin\n" );
		return usage();
	}

	return 0;
}


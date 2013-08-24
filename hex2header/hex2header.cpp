#include <stdio.h>
#include "../util/read_hex.hpp"
#include "../util/str.hpp"

//------------------------------------------------------------------------------
int usage()
{
	printf( "Usage: hex2header <infile.hex> <outfile.h>\n\n" );
	return -1;
}

//------------------------------------------------------------------------------
int main( int argn, char *argv[] )
{
	if ( argn != 3 )
	{
		return usage();
	}

	Cstr buffer;
	if ( !buffer.fileToBuffer( argv[1] ) )
	{
		printf( "Could not load [%s]\n", argv[1] );
		return usage();
	}

	CLinkList<ReadHex::Chunk> chunklist;
	ReadHex::parse( chunklist, buffer.c_str(), buffer.size() );

	ReadHex::Chunk *chunk = chunklist.getFirst();

	buffer.format( "unsigned int g_origin = 0x%04X;\n\n"
				   "unsigned int g_size = 0x%04X;\n\n"
				   "const PROGMEM int g_data[]=\n{", chunk->addressBase, chunk->size );

	for( unsigned int i=0; i<chunk->size; i += 2  )
	{
		if ( !(i%16) )
		{
			buffer.append( "\n\t" );
		}
		buffer.appendFormat( "0x%02X%02X, ", chunk->data[i+1], chunk->data[i] );
	}
			
	buffer.append( "\n};\n" );

	buffer.bufferToFile( argv[2] );

	printf( "[%s] -> [%s] complete\n", argv[1], argv[2] );

	return 0;
}


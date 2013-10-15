#include "font.hpp"

#include <png.h>

#include "../util/str.hpp"

//------------------------------------------------------------------------------
bool Font::load( const char* metricsName )
{
	m_valid = false;
	memset( m_metrics, 0, sizeof(Metrics)*95 );

	Cstr metricsFileName = metricsName;
	metricsFileName += ".fnt";
	Cstr metricsFile;
	if ( !metricsFile.fileToBuffer(metricsFileName) )
	{
		printf( "could not read metrics file [%s]\n", metricsFileName.c_str() );
		return false;
	}

	int offset = metricsFile.find( "Bitmap=" );
	if ( offset == -1 )
	{
		printf( "Could not find bitmap name\n" );
		return false;
	}
	const char* bitmapName = metricsFile.c_str() + offset + 7;

	Cstr gray;
	for( int i=0; *bitmapName != '.' ; i++ )
	{
		gray += *bitmapName++;
	}

	Cstr png = gray;
	png += ".png";
	gray += ".gray";

	if ( !m_bitmap.fileToBuffer(gray) )
	{
		printf( "could not load [%s]\n", gray.c_str() );
		return false;
	}

	// dump the bimap
	printf( "Using: %s\n", gray.c_str() );

	FILE* pngFile = fopen( png, "rb" );
	if ( !pngFile )
	{
		printf( "could not load [%s]\n", png.c_str() );
		return false;
	}

	png_byte sig[8];
	fread( sig, 1, 8, pngFile );
	if ( !png_check_sig(sig, 8) )
	{
		printf( "[%s] bad signature\n", png.c_str() );
		return false;
	}

	png_structp png_ptr = png_create_read_struct( PNG_LIBPNG_VER_STRING, NULL, NULL, NULL );
	png_infop info_ptr = png_create_info_struct( png_ptr );
	png_init_io( png_ptr, pngFile );
	png_set_sig_bytes( png_ptr, 8 );
	png_read_info( png_ptr, info_ptr );

	int depth, color;
	png_uint_32 height, width;
	png_get_IHDR( png_ptr, info_ptr, &width, &height, &depth, &color, NULL, NULL, NULL);

	m_height = height;
	m_width = width;

	printf( "resolution: %dx%d\n", m_width, m_height );
	
	m_fontHeight = 0;

	offset = metricsFile.find( "Char=" );
	if ( offset == -1 )
	{
		printf( "Could not find a Char=\n" );
		return false;
	}

	for( int character = 0; offset < (int)metricsFile.size(); character++ )
	{
		for( ;offset < (int)metricsFile.size() && metricsFile[offset] != '\"'; offset++ );

		offset += 4;
		
		if ( offset < (int)metricsFile.size() )
		{
			if ( sscanf( metricsFile.c_str() + offset, "%d,%d,%d,%d,%d,%d",
						 &m_metrics[character].x,
						 &m_metrics[character].y,
						 &m_metrics[character].w,
						 &m_metrics[character].h,
						 &m_metrics[character].pre,
						 &m_metrics[character].post ) != 6 )
			{
				break;
			}

/*
			printf( "[%c] %d %d %d %d %d %d\n", (char)(character + 32),
					m_metrics[character].x,
					m_metrics[character].y,
					m_metrics[character].w,
					m_metrics[character].h,
					m_metrics[character].pre,
					m_metrics[character].post );

			for( int y=m_metrics[character].y; y<m_metrics[character].y + m_metrics[character].h; y++ )
			{
				for( int x=m_metrics[character].x; x<m_metrics[character].x + m_metrics[character].w; x++ )
				{
					printf( "%c", m_bitmap[x + y*128] ? '.' : ' ' );
				}
				printf( "\n" );
			}
*/			
			if ( m_metrics[character].h > m_fontHeight )
			{
				m_fontHeight = m_metrics[character].h;
			}
		}

		for( ;offset < (int)metricsFile.size() && metricsFile[offset] != '\n'; offset++ );
	}

	return true;
}
/*
//------------------------------------------------------------------------------
const unsigned int Font::getPrintLength( const char* string )
{
	int length = 0;
	for( int i=0; string[i]; i++ )
	{
		if ( string[i] >=32 && string[i] < 127 )
		{
			unsigned char c = string[i] - 32;
			length += m_metrics[c].pre + m_metrics[c].post + m_metrics[c].w;

		}
	}

	return length;
}
*/

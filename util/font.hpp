#ifndef FONT_HPP
#define FONT_HPP
/*----------------------------------------------------------------------------*/

#include "str.hpp"

//------------------------------------------------------------------------------
class Font
{
public:

	bool load( const char* metricsName );
	const bool valid() const { return m_valid; }
	
	static const char glyphFromChar( const char c ) { return c - 32; }

	struct Metrics
	{
		int x;
		int y;
		int w;
		int h;
		int pre;
		int post;
	};

	const int getHeight() const { return m_fontHeight; }
	const Metrics* getMetrics() const { return m_metrics; }
	const unsigned char* getBitmap( int &width, int &height ) const { width = m_width, height = m_height; return (const unsigned char*)m_bitmap.c_str(); }
	const unsigned int getPrintLength( const char* string );

	Font() { m_valid = false; }
	
private:

	Metrics m_metrics[95]; // 126 - 32 :: '~' - ' ' .. fonts only convert 94 characters

	Cstr m_bitmap;
	int m_fontHeight;
	bool m_valid;
	int m_width;
	int m_height;
};

#endif

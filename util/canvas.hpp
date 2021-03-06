#ifndef CANVAS_HPP
#define CANVAS_HPP
/*------------------------------------------------------------------------------*/
/* Copyright: (c) 2013 by Curt Hartung
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

struct CanvasImplementation;

#define CANVAS_RGB(r,g,b) (((r)<<16)|((g)<<8)|(b)|0xFF000000)
#define CANVAS_INVERT(rgb) ((rgb) ^ 0xFFFFFF00)
#define CANVAS_RGBA(r,g,b,a) (((a)<<24)|((r)<<16)|((g)<<8)|(b))
#define CANVAS_GET_BLUE(color) ((color) & 0x000000FF)
#define CANVAS_GET_GREEN(color) ((color)>>8 & 0x000000FF)
#define CANVAS_GET_RED(color) ((color)>>16 & 0x000000FF)
#define CANVAS_GET_ALPHA(color) ((color)>>24 & 0x000000FF)

//------------------------------------------------------------------------------
class Canvas
{
public:

	bool init( const unsigned int sizeX,
			   const unsigned int sizeY,
			   const unsigned int originX,
			   const unsigned int originY  );
	
	void blit( void* hdc );
	void getBounds( unsigned int *width, unsigned int *height ) { *width = m_sizeX; *height = m_sizeY; }
	void clear();
	
	int set( unsigned int x, unsigned int y, unsigned int rgba );

	void drawLine( int p1x, int p1y, int p2x, int p2y, int color );

	const int* getBuffer() const { return m_buffer; }

	Canvas();
	~Canvas();
	
private:

	int* m_buffer;
	unsigned int m_originX;
	unsigned int m_originY;
	unsigned int m_sizeX;
	unsigned int m_sizeY;
	unsigned int m_sizeXx2;
	int m_pixels;

	CanvasImplementation* m_imp;
};
#endif

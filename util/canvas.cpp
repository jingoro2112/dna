/* Copyright: (c) 2013 by Curt Hartung
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

// so I wrote this for a tank-fighting game and have used it quite a
// few more times after that

#include <stdafx.h>
#include "canvas.hpp"

#include <tchar.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

//------------------------------------------------------------------------------
struct CanvasImplementation
{
	HDC bmpDC;
	HBITMAP backbuffer;
	BITMAPINFO *bmi;
};

//------------------------------------------------------------------------------
Canvas::Canvas()
{
	m_imp = new CanvasImplementation;
	memset( m_imp, 0, sizeof(CanvasImplementation) );

	m_buffer = 0;
}

//------------------------------------------------------------------------------
Canvas::~Canvas()
{
	delete m_imp;
	delete[] m_buffer;
}

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

//------------------------------------------------------------------------------
bool Canvas::init( const unsigned int sizeX,
				   const unsigned int sizeY,
				   const unsigned int originX,
				   const unsigned int originY  )
{
	m_sizeX = sizeX;
	m_sizeXx2 = sizeX*2;
	m_sizeY = sizeY;
	m_pixels = sizeX * sizeY;
	m_buffer = new int[ m_pixels ];

	m_originX = originX;
	m_originY = originY;

	m_imp->bmpDC = CreateCompatibleDC(0);
	m_imp->backbuffer = CreateBitmap( m_sizeX, m_sizeY, 1, 32, 0 );
	SelectObject( m_imp->bmpDC, m_imp->backbuffer );

	m_imp->bmi = (BITMAPINFO *)(new char[ sizeof(BITMAPINFOHEADER) + 256*sizeof(RGBQUAD) ]);
	memset( &m_imp->bmi->bmiHeader, 0, sizeof(BITMAPINFOHEADER));
	m_imp->bmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);	
	GetDIBits( m_imp->bmpDC, m_imp->backbuffer, 0, m_sizeY, 0, m_imp->bmi, DIB_RGB_COLORS );
	GetDIBits( m_imp->bmpDC, m_imp->backbuffer, 0, m_sizeY, m_buffer, m_imp->bmi, DIB_RGB_COLORS );

	return true;
}

//------------------------------------------------------------------------------
void Canvas::clear()
{
	memset( m_buffer, 0, m_pixels * sizeof(int) );
}

//------------------------------------------------------------------------------
int Canvas::set( unsigned int x, unsigned int y, unsigned int rgba )
{
	if ( (x >= m_sizeX) || (y >= m_sizeY) ) 
	{
		return false;
	}

	if ( (rgba & 0xFF000000) != 0xFF000000 )
	{
		unsigned int pixel = ((m_sizeY - (y+1)) * m_sizeX) + x;
		unsigned int pixrgb = m_buffer[pixel];

		unsigned int b1 = pixrgb & 0xFF;
		pixrgb >>= 8;
		unsigned int g1 = pixrgb & 0xFF;
		pixrgb >>= 8;
		unsigned int r1 = pixrgb & 0xFF;

		int b2 = rgba & 0xFF;
		rgba >>= 8;
		unsigned int g2 = rgba & 0xFF;
		rgba >>= 8;
		unsigned int r2 = rgba & 0xFF;
		rgba >>= 8;

		unsigned int inv = 255 - rgba;
		m_buffer[pixel] = CANVAS_RGB((r2 * rgba + r1 * inv) / 255,
									 (g2 * rgba + g1 * inv) / 255,
									 (b2 * rgba + b1 * inv) / 255);

		
	}
	else
	{
		m_buffer[((m_sizeY - (y+1)) * m_sizeX) + x] = rgba;
	}

	return true;
}

//------------------------------------------------------------------------------
void Canvas::drawLine( int p1x, int p1y, int p2x, int p2y, int color )
{
	// horizontal line
	if (p1x == p2x)
	{
		if (p1y > p2y)
		{
			for( ; p1y >= p2y ; p1y-- )
			{
				if ( !set(p1x, p1y, color) )
				{
					return;
				}
			}
		}
		else
		{
			for( ; p1y <= p2y ; p1y++ )
			{
				if ( !set(p1x, p1y, color) )
				{
					return;
				}
			}
		}

		return;
	}

	// vertical line
	else if (p1y == p2y)
	{
		if (p1x > p2x)
		{
			for( ; p1x >= p2x ; p1x-- )
			{
				if ( !set(p1x, p1y, color) )
				{
					return;
				}
			}
		}
		else
		{
			for( ; p1x <= p2x ; p1x++ )
			{
				if ( !set(p1x, p1y, color) )
				{
					return;
				}
			}
		}

		return;
	}

	int dy = p2y - p1y;
	int dx = p2x - p1x;

	if ( dx == dy || dx == -dy ) // diagonal line?
	{
		if ( p1x > p2x )
		{
			if ( p1y > p2y )
			{
				for( ; p1y >= p2y ; p1y--, p1x-- )
				{
					if ( !set(p1x, p1y, color) )
					{
						return;
					}
				}
			}
			else
			{
				for( ; p1y <= p2y ; p1y++, p1x-- )
				{
					if ( !set(p1x, p1y, color) )
					{
						return;
					}
				}
			}
		}
		else
		{
			if ( p1y > p2y )
			{
				for( ; p1y >= p2y ; p1y--, p1x++ )
				{
					if ( !set(p1x, p1y, color) )
					{
						return;
					}
				}
			}
			else
			{
				for( ; p1y <= p2y ; p1y++, p1x++ )
				{
					if ( !set(p1x, p1y, color) )
					{
						return;
					}
				}
			}
		}
	}

	if ( dx < 0 )
	{
		dx = -dx;
	}

	if ( dy < 0 )
	{
		dy = -dy;
	}

	int err = dx - dy;
	if ( p1x < p2x )
	{
		if ( p1y < p2y )
		{
			for(;;)
			{
				if ( !set(p1x, p1y, color) )
				{
					return;
				}	

				if ( p1x == p2x && p1y == p2y )
				{
					break;
				}

				int e2 = 2 * err;
				if ( e2 > -dy )
				{
					err -= dy;
					p1x++;
				}

				if ( e2 < dx )
				{
					err += dx;
					p1y++;
				}
			}
		}
		else
		{
			for(;;)
			{
				if ( !set(p1x, p1y, color) )
				{
					return;
				}	

				if ( p1x == p2x && p1y == p2y )
				{
					break;
				}

				int e2 = 2 * err;
				if ( e2 > -dy )
				{
					err -= dy;
					p1x++;
				}

				if ( e2 < dx )
				{
					err += dx;
					p1y--;
				}
			}
		}
	}
	else
	{
		if ( p1y < p2y )
		{
			for(;;)
			{
				if ( !set(p1x, p1y, color) )
				{
					return;
				}	

				if ( p1x == p2x && p1y == p2y )
				{
					break;
				}

				int e2 = 2 * err;
				if ( e2 > -dy )
				{
					err -= dy;
					p1x--;
				}

				if ( e2 < dx )
				{
					err += dx;
					p1y++;
				}
			}
		}
		else
		{
			for(;;)
			{
				if ( !set(p1x, p1y, color) )
				{
					return;
				}	

				if ( p1x == p2x && p1y == p2y )
				{
					break;
				}

				int e2 = 2 * err;
				if ( e2 > -dy )
				{
					err -= dy;
					p1x--;
				}

				if ( e2 < dx )
				{
					err += dx;
					p1y--;
				}
			}
		}
	}
}

//------------------------------------------------------------------------------
void Canvas::blit( void* hdc )
{
	SetDIBits( m_imp->bmpDC, m_imp->backbuffer, 0, m_sizeY, m_buffer, m_imp->bmi, DIB_RGB_COLORS );
	BitBlt( (HDC)hdc, m_originX, m_originY, m_sizeX, m_sizeY, m_imp->bmpDC, 0, 0, SRCCOPY);
}

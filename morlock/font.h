#ifndef FONT_H
#define FONT_H
/*------------------------------------------------------------------------------*/
/* Copyright: (c) 2013 by Curt Hartung
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

//------------------------------------------------------------------------------
struct MFont
{
	int x;
	int y;
	int w;
	int h;
	int pre;
	int post;
};

#endif
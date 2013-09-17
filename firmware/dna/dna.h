#ifndef DNA_H
#define DNA_H
/*------------------------------------------------------------------------------*/
/* Copyright: (c) 2013 by Curt Hartung
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#if !defined(DNA) && !defined(OLED) && !defined(PROTO88) && !defined(DNAPROTO)
#error Must define a hardware platform
#endif

// not using typedef sine C-ish libraris #DEFINE this and that collides
// when include-order matters
#ifndef uchar
#define uchar unsigned char
#endif
#ifndef uint
#define uint unsigned int
#endif

#include "dna_defs.h" // is included in c++ projects as well so need sto remain generic

#endif

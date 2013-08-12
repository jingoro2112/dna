#ifndef HASH_H
#define HASH_H
//------------------------------------------------------------------------------
/* Copyright: (c) 2013 by Curt Hartung
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#include <ctype.h>

//------------------------------------------------------------------------------
namespace Hash
{

// "Pre" is the pre-hash value and is used to compute running hashes
// 'case' versions are case insensitive

// very fast hash (hashes 32-bits at a time)
inline unsigned int hash( const void *dat, const unsigned int len, const unsigned int pre =0x811C9DC5 );
inline unsigned int hashStr( const void *dat, const unsigned int pre =0x811C9DC5 );
inline unsigned int hashStrCase( const void *dat, const unsigned int pre =0x811C9DC5 );

// lookup-table hash 1
inline unsigned int hash1( const void *dat, const unsigned int len, const unsigned int pre =0 );
inline unsigned int hash1Str( const void *dat, const unsigned int pre =0 );
inline unsigned int hash1StrCase( const void *dat, const unsigned int pre =0 );
	
// lookup-table hash 2
inline unsigned int hash2( const void *dat, const unsigned int len, const unsigned int pre =0 );
inline unsigned int hash2Str( const void *dat, const unsigned int pre =0 );
inline unsigned int hash2StrCase( const void *dat, const unsigned int pre =0 );

// FNV hash
inline unsigned int fnv32( const void *dat, const unsigned int len, const unsigned int pre =0x811C9DC5 );
inline unsigned int fnv32Str( const void *dat, const unsigned int pre =0x811C9DC5 );
inline unsigned int fnv32StrCase( const void *dat, const unsigned int pre =0x811C9DC5 );
inline unsigned long long fnv64( const void *dat, const unsigned int len, const unsigned long long pre =0xCBF29CE484222325ULL );
inline unsigned long long fnv64Str( const void *dat, const unsigned long long pre =0xCBF29CE484222325ULL );
inline unsigned long long fnv64StrCase( const void *dat, const unsigned long long pre =0xCBF29CE484222325ULL );

//------------------------------------------------------------------------------
static const unsigned int table1[128] =
{
	0x00000000, 0x48000000, 0x24000000, 0x6C000000,	0x12000000, 0x5A000000, 0x36000000, 0x7E000000,
	0x09000000, 0x41000000, 0x2D000000, 0x65000000,	0x1B000000, 0x53000000, 0x3F000000, 0x77000000,
	0x04800000, 0x4C800000, 0x20800000, 0x68800000,	0x16800000, 0x5E800000, 0x32800000, 0x7A800000,
	0x0D800000, 0x45800000, 0x29800000, 0x61800000,	0x1F800000, 0x57800000, 0x3B800000, 0x73800000,
	0x02400000, 0x4A400000, 0x26400000, 0x6E400000,	0x10400000, 0x58400000, 0x34400000, 0x7C400000,
	0x0B400000, 0x43400000, 0x2F400000, 0x67400000,	0x19400000, 0x51400000, 0x3D400000, 0x75400000,
	0x06C00000, 0x4EC00000, 0x22C00000, 0x6AC00000,	0x14C00000, 0x5CC00000, 0x30C00000, 0x78C00000,
	0x0FC00000, 0x47C00000, 0x2BC00000, 0x63C00000,	0x1DC00000, 0x55C00000, 0x39C00000, 0x71C00000,
	0x01200000, 0x49200000, 0x25200000, 0x6D200000,	0x13200000, 0x5B200000, 0x37200000, 0x7F200000,
	0x08200000, 0x40200000, 0x2C200000, 0x64200000,	0x1A200000, 0x52200000, 0x3E200000, 0x76200000,
	0x05A00000, 0x4DA00000, 0x21A00000, 0x69A00000,	0x17A00000, 0x5FA00000, 0x33A00000, 0x7BA00000,
	0x0CA00000, 0x44A00000, 0x28A00000, 0x60A00000,	0x1EA00000, 0x56A00000, 0x3AA00000, 0x72A00000,
	0x03600000, 0x4B600000, 0x27600000, 0x6F600000,	0x11600000, 0x59600000, 0x35600000, 0x7D600000,
	0x0A600000, 0x42600000, 0x2E600000, 0x66600000,	0x18600000, 0x50600000, 0x3C600000, 0x74600000,
	0x07E00000, 0x4FE00000, 0x23E00000, 0x6BE00000,	0x15E00000, 0x5DE00000, 0x31E00000, 0x79E00000,
	0x0EE00000, 0x46E00000, 0x2AE00000, 0x62E00000,	0x1CE00000, 0x54E00000, 0x38E00000, 0x70E00000
};

//------------------------------------------------------------------------------
static const unsigned int table2[128] =
{
	0x00000000, 0xF5000000, 0x7A800000, 0x8F800000,	0x3D400000, 0xC8400000, 0x47C00000, 0xB2C00000,
	0x1EA00000, 0xEBA00000, 0x64200000, 0x91200000,	0x23E00000, 0xD6E00000, 0x59600000, 0xAC600000,
	0x0F500000, 0xFA500000, 0x75D00000, 0x80D00000,	0x32100000, 0xC7100000, 0x48900000, 0xBD900000,
	0x11F00000, 0xE4F00000, 0x6B700000, 0x9E700000,	0x2CB00000, 0xD9B00000, 0x56300000, 0xA3300000,
	0x07A80000, 0xF2A80000, 0x7D280000, 0x88280000,	0x3AE80000, 0xCFE80000, 0x40680000, 0xB5680000,
	0x19080000, 0xEC080000, 0x63880000, 0x96880000,	0x24480000, 0xD1480000, 0x5EC80000, 0xABC80000,
	0x08F80000, 0xFDF80000, 0x72780000, 0x87780000,	0x35B80000, 0xC0B80000, 0x4F380000, 0xBA380000,
	0x16580000, 0xE3580000, 0x6CD80000, 0x99D80000,	0x2B180000, 0xDE180000, 0x51980000, 0xA4980000,
	0x03D40000, 0xF6D40000, 0x79540000, 0x8C540000,	0x3E940000, 0xCB940000, 0x44140000, 0xB1140000,
	0x1D740000, 0xE8740000, 0x67F40000, 0x92F40000,	0x20340000, 0xD5340000, 0x5AB40000, 0xAFB40000,
	0x0C840000, 0xF9840000, 0x76040000, 0x83040000,	0x31C40000, 0xC4C40000, 0x4B440000, 0xBE440000,
	0x12240000, 0xE7240000, 0x68A40000, 0x9DA40000,	0x2F640000, 0xDA640000, 0x55E40000, 0xA0E40000,
	0x047C0000, 0xF17C0000, 0x7EFC0000, 0x8BFC0000,	0x393C0000, 0xCC3C0000, 0x43BC0000, 0xB6BC0000,
	0x1ADC0000, 0xEFDC0000, 0x605C0000, 0x955C0000,	0x279C0000, 0xD29C0000, 0x5D1C0000, 0xA81C0000,
	0x0B2C0000, 0xFE2C0000, 0x71AC0000, 0x84AC0000,	0x366C0000, 0xC36C0000, 0x4CEC0000, 0xB9EC0000,
	0x158C0000, 0xE08C0000, 0x6F0C0000, 0x9A0C0000,	0x28CC0000, 0xDDCC0000, 0x524C0000, 0xA74C0000
};

//------------------------------------------------------------------------------
unsigned int hash( const void *dat, const unsigned int len, const unsigned int pre )
{
	if ( !len )
	{
		return 0;
	}

	unsigned char *data = (unsigned char *)dat;
	unsigned int hash = pre;
	unsigned int tmp;
	unsigned int rem = len & 3;
	unsigned int length = len >> 2;

	for ( ; length > 0; length-- )
	{
		hash += *((const unsigned short *)data);
		tmp = (*((const unsigned short *)(data + 2)) << 11) ^ hash;
		hash = (hash << 16) ^ tmp;
		data +=  sizeof(unsigned int);
		hash += hash >> 11;
	}

	switch (rem)
	{
		case 3:
		{
			hash += *((const unsigned short *)data);
			hash ^= hash << 16;
			hash ^= data[2] << 18;
			hash += hash >> 11;
			break;
		}
		case 2:
		{
			hash += *((const unsigned short *)data);
			hash ^= hash << 11;
			hash += hash >> 17;
			break;
		}
		case 1:
		{
			hash += *data;
			hash ^= hash << 10;
			hash += hash >> 1;
		}
	}

	hash ^= hash << 3;
	hash += hash >> 5;
	hash ^= hash << 4;
	hash += hash >> 17;
	hash ^= hash << 25;
	hash += hash >> 6;
	return hash;
}

//------------------------------------------------------------------------------
unsigned int hashStr( const void *dat, const unsigned int pre )
{
	if ( !dat )
	{
		return 0;
	}

	unsigned char *data = (unsigned char *)dat;
	unsigned int hash = pre;
	unsigned int tmp = 0;

	while( data[0] && data[1] && data[2] && data[3] )
	{
		hash += *((const unsigned short *)data);
		tmp = (*((const unsigned short *)(data + 2)) << 11) ^ hash;
		hash = (hash << 16) ^ tmp;
		data +=  sizeof(unsigned int);
		hash += hash >> 11;
	}

	if ( data[0] ) // was there remainder?
	{
		if ( !data[1] )
		{
			hash += *data;
			hash ^= hash << 10;
			hash += hash >> 1;
		}
		else if ( !data[2] )
		{
			hash += *((const unsigned short *)data);
			hash ^= hash << 11;
			hash += hash >> 17;
		}
		else
		{
			hash += *((const unsigned short *)data);
			hash ^= hash << 16;
			hash ^= data[2] << 18;
			hash += hash >> 11;
		}
	}

	hash ^= hash << 3;
	hash += hash >> 5;
	hash ^= hash << 4;
	hash += hash >> 17;
	hash ^= hash << 25;
	hash += hash >> 6;
	return hash;
}

//------------------------------------------------------------------------------
unsigned int hashStrCase( const void *dat, const unsigned int pre )
{
	if ( !dat )
	{
		return 0;
	}

	unsigned char *data = (unsigned char *)dat;
	unsigned int hash = pre;
	unsigned int tmp = 0;

	while( data[0] && data[1] && data[2] && data[3] )
	{
		hash += (toupper(data[1]) << 8) | toupper(data[0]);
		tmp = (((toupper(data[3]) << 8) | toupper(data[2])) << 11) ^ hash;
		hash = (hash << 16) ^ tmp;
		data +=  sizeof(unsigned int);
		hash += hash >> 11;
	}

	if ( data[0] ) // was there remainder?
	{
		if ( !data[1] )
		{
			hash += toupper( *data );
			hash ^= hash << 10;
			hash += hash >> 1;
		}
		else if ( !data[2] )
		{
			hash += (toupper(data[1]) << 8) | toupper(data[0]);
			hash ^= hash << 11;
			hash += hash >> 17;
		}
		else
		{
			hash += (toupper(data[1]) << 8) | toupper(data[0]);
			hash ^= hash << 16;
			hash ^= toupper(data[2]) << 18;
			hash += hash >> 11;
		}
	}

	hash ^= hash << 3;
	hash += hash >> 5;
	hash ^= hash << 4;
	hash += hash >> 17;
	hash ^= hash << 25;
	hash += hash >> 6;
	return hash;
}

//------------------------------------------------------------------------------
unsigned int hash1( const void *dat, const unsigned int len, const unsigned int pre )
{
	unsigned int length = len;
	unsigned int hash = pre;
	unsigned char *data = (unsigned char *)dat;
	while ( length-- )
	{
		hash = (hash >> 7) ^ table1[(hash ^ (*data++)) & 0x7f];
	}
	
	return hash;
}

//------------------------------------------------------------------------------
unsigned int hash1Str( const void *dat, const unsigned int pre )
{
	unsigned int hash = pre;
	unsigned char *data = (unsigned char *)dat;
	while ( *(unsigned char *)data )
	{
		hash = (hash >> 7) ^ table1[(hash ^ (*data++)) & 0x7f];
	}

	return hash;
}

//------------------------------------------------------------------------------
unsigned int hash1StrCase( const void *dat, const unsigned int pre )
{
	unsigned int hash = pre;
	unsigned char *data = (unsigned char *)dat;
	while ( *(unsigned char *)data )
	{
		hash = (hash >> 7) ^ table1[(hash ^ toupper(*data++)) & 0x7f];
	}

	return hash;
}

//------------------------------------------------------------------------------
unsigned int hash2( const void *dat, const unsigned int len, const unsigned int pre )
{
	unsigned int hash = pre;
	unsigned int length = len;
	unsigned char *data = (unsigned char *)dat;
	while ( length-- )
	{
		hash = (hash >> 7) ^ table2[(hash ^ (*data++)) & 0x7f];
	}
	return hash;
}

//------------------------------------------------------------------------------
unsigned int hash2Str( const void *dat, const unsigned int pre )
{
	unsigned int hash = pre;
	unsigned char *data = (unsigned char *)dat;
	while ( *(unsigned char *)data )
	{
		hash = (hash >> 7) ^ table2[(hash ^ (*data++)) & 0x7f];
	}
	return hash;
}

//------------------------------------------------------------------------------
unsigned int hash2StrCase( const void *dat, const unsigned int pre )
{
	unsigned int hash = pre;
	unsigned char *data = (unsigned char *)dat;
	while ( *(unsigned char *)data )
	{
		hash = (hash >> 7) ^ table2[(hash ^ toupper(*data++)) & 0x7f];
	}
	return hash;
}

/*

32 bit FNV_prime = 224 + 28 + 0x93 = 16777619
64 bit FNV_prime = 240 + 28 + 0xb3 = 1099511628211
128 bit FNV_prime = 288 + 28 + 0x3b = 309485009821345068724781371
256 bit FNV_prime = 2168 + 28 + 0x63 = 374144419156711147060143317175368453031918731002211
512 bit FNV_prime = 2344 + 28 + 0x57 =
35835915874844867368919076489095108449946327955754392558399825615420669938882575
126094039892345713852759
1024 bit FNV_prime = 2680 + 28 + 0x8d =
50164565101131186554345988110352789550307653454047907443030175238311120551081474
51509157692220295382716162651878526895249385292291816524375083746691371804094271
873160484737966720260389217684476157468082573 

Part of the magic of FNV is the selection of the FNV_prime for a given sized unsigned integer.
Some primes do hash better than other primes for a given integer size.
The offset_basis for FNV-1 is dependent on n, the size of the hash:

32 bit offset_basis = 2166136261
64 bit offset_basis = 14695981039346656037
128 bit offset_basis = 144066263297769815596495629667062367629
256 bit offset_basis = 100029257958052580907070968620625704837092796014241193945225284501741471925557
512 bit offset_basis =
96593031294966694980094354007163104660904187456726378961083743294344626579945829
32197716438449813051892206539805784495328239340083876191928701583869517785
1024 bit offset_basis =
14197795064947621068722070641403218320880622795441933960878474914617582723252296
73230371772215086409652120235554936562817466910857181476047101507614802975596980
40773201576924585630032153049571501574036444603635505054127112859663616102678680
82893823963790439336411086884584107735010676915

*/

//------------------------------------------------------------------------------
unsigned int fnv32( const void *dat, const unsigned int len, unsigned int pre )
{
	unsigned int length = len;
	unsigned char *data = (unsigned char *)dat;
	while ( length-- )
	{
		pre ^= (unsigned int)(*data++);
		pre *= 0x1000193;
	}

	return pre;
}

//------------------------------------------------------------------------------
unsigned int fnv32Str( const void *dat, unsigned int pre )
{
	unsigned char *data = (unsigned char *)dat;
	while ( *(unsigned char *)data )
	{
		pre ^= (unsigned int)(*data++);
		pre *= 0x1000193;
	}

	return pre;
}

//------------------------------------------------------------------------------
unsigned int fnv32StrCase( const void *dat, unsigned int pre )
{
	unsigned char *data = (unsigned char *)dat;
	while ( *(unsigned char *)data )
	{
		pre ^= (unsigned int)toupper(*data++);
		pre *= 0x1000193;
	}

	return pre;
}

//------------------------------------------------------------------------------
unsigned long long fnv64( const void *dat, const unsigned int len, unsigned long long pre )
{
	unsigned int length = len;
	unsigned char *data = (unsigned char *)dat;
	while ( length-- )
	{
		pre ^= (unsigned long long)(*data++);
		pre *= 0x100000001b3ULL;
	}

	return pre;
}

//------------------------------------------------------------------------------
unsigned long long fnv64Str( const void *dat, unsigned long long pre )
{
	unsigned char *data = (unsigned char *)dat;
	while ( *(unsigned char *)data )
	{
		pre ^= (unsigned long long)(*data++);
		pre *= 0x100000001b3ULL;
	}

	return pre;
}

//------------------------------------------------------------------------------
unsigned long long fnv64StrCase( const void *dat, unsigned long long pre )
{
	unsigned char *data = (unsigned char *)dat;
	while ( *(unsigned char *)data )
	{
		pre ^= (unsigned long long)toupper(*data++);
		pre *= 0x100000001b3ULL;
	}

	return pre;
}
					  
}

#endif

#ifndef JSON_PARSER_H
#define JSON_PARSER_H
/*------------------------------------------------------------------------------*/

#include "str.hpp"
#include "LinkHash.hpp"
#include "hash.hpp"

#include <stdlib.h>
#include <stdio.h>

//------------------------------------------------------------------------------
class JsonValue
{
public:

	inline int read( const char* buf );

	inline void write( Cstr& str );
	
	inline void writePretty( Cstr& str );

	bool valid() const { return m_valid; }

	enum types
	{
		JNULL,
		JBOOL,
		JINT,
		JSTRING,
		JFLOAT,
		JOBJECT,
		JLIST,
	};
	const int getType() const { return m_type; }

	void clear() { m_str = "null"; if ( m_values ) m_values->clear(); m_type = JsonValue::JNULL; m_index = 0; m_valid = true; }

	long long asInt() const { return m_int; }
	double asFloat() const { return m_float; }
	bool asBool() const { return m_int ? true : false; }
	const char* asUTF8Str( int* size =0 ) const { if (size) *size = m_str.size(); return m_str.c_str(); }

	inline const char* get( const char* key, const char* def, char* buf =0, int maxSize =0 );
	inline const char* get( int index, const char* def, char* buf =0, int maxSize =0 );
	
	inline JsonValue* get( const char* key ) { return m_type == JsonValue::JOBJECT ? m_values->get( Hash::fnv64Str(key) ) : 0; }
	inline JsonValue* get( int index ) { return m_type == JsonValue::JLIST ? m_values->get(index) : 0; }

	operator const char*() const { return m_str.c_str(); }

	inline JsonValue& object(); 
	inline JsonValue& list();

	JsonValue& set( const char* val ) { m_type = JsonValue::JSTRING; m_str = val; return *this; }
	JsonValue& set( const long long val ) { m_type = JsonValue::JINT; m_int = val; m_float = (double)val; m_str.format("%lld",val); return *this; }
	JsonValue& set( const bool val ) { m_type = JsonValue::JBOOL; m_int = val ? true:false; m_str = val ? "true":"false"; return *this; }
	JsonValue& set() { m_type = JsonValue::JNULL; m_str = "null"; return *this; }
	JsonValue& set( const double val ) { m_type = JsonValue::JFLOAT; m_int = (long long)val; m_float = val; m_str.format("%f",val); return *this; }

	JsonValue& add( const char* key, const char* val ) { return checkAdd(key)->set( val ); }
	JsonValue& add( const char* key, const long long val ) { return checkAdd(key)->set( val ); }
	JsonValue& add( const char* key, const bool val ) { return checkAdd(key)->set( val ); }
	JsonValue& add( const char* key, double val ) { return checkAdd(key)->set( val ); }
	JsonValue& add( const char* check ) { if (m_type == JsonValue::JOBJECT) return checkAdd(check)->set(); else return checkAdd()->set(check); }
	JsonValue& add( const long long val ) { return checkAdd()->set( val ); }
	JsonValue& add( const bool val ) { return checkAdd()->set( val ); }
	JsonValue& add( const double val ) { return checkAdd()->set( val ); }
	JsonValue& add() { return checkAdd()->set(); }

	JsonValue& operator=( const char* val ) { return set(val); }
	JsonValue& operator=( const long long val ) { return set(val); }
	JsonValue& operator=( const bool val ) { return set(val); }
	JsonValue& operator=( const double val ) { return set(val); }
	JsonValue& operator=( const int val ) { return set((long long)val); }
	JsonValue& operator=( const unsigned int val ) { return set((long long)val); }
	JsonValue& operator=( const long val ) { return set((long long)val); }
	JsonValue& operator=( const unsigned long val ) { return set((long long)val); }
	
	inline void UTF8toJsonUnicode( const char* in, Cstr& out );
	inline int strJsonUnicodeToUTF8( const char* in, Cstr& out );
	inline int unicodeToUTF8( const unsigned int unicode, char* out, const unsigned int maxLen );

	JsonValue& operator[]( const char* key ) { return *checkAdd(key); }
	JsonValue& operator[]( const int index ) { return *checkAdd(index); }

	JsonValue& operator+=( const char* check ) { if (m_type == JsonValue::JOBJECT) return checkAdd(check)->set(); else return checkAdd()->set(check); }
	JsonValue& operator+=( const long long val ) { return checkAdd()->set(val); }
	JsonValue& operator+=( const bool val ) { return checkAdd()->set(val); }
	JsonValue& operator+=( const double val ) { return checkAdd()->set(val); }

	JsonValue() { m_values = 0; clear(); }
	JsonValue( const char* data ) { m_values = 0; clear(); read(data); }
	~JsonValue() { clear(); delete m_values; }

private:

	inline int readRawValue( const char* buf );
	void addValues() { if (!m_values) m_values = new CLinkHash<JsonValue>; }

	JsonValue( const JsonValue& j ) {} // not defined yet
	

	inline JsonValue* checkAdd( const char* key );
	inline JsonValue* checkAdd( const int index =-1 );

	bool m_valid;
	long long m_int;
	double m_float;
	int m_type;
	Cstr m_key;
	int m_index;
	Cstr m_str;
	CLinkHash<JsonValue> *m_values;
};

//------------------------------------------------------------------------------
const char* JsonValue::get( const char* key, const char* def, char* buf, int maxSize )
{
	JsonValue* val = 0;
	if ( m_type == JsonValue::JOBJECT )
	{
		val = m_values->get( Hash::fnv64Str(key) );
	}

	if ( !val )
	{
		if ( buf )
		{
			strncpy( buf, def, maxSize ? maxSize : 2000000000);
			return buf;
		}
		else
		{
			return def;
		}
	}
	else
	{
		if ( buf )
		{
			strncpy( buf, val->m_str, maxSize ? maxSize : 2000000000);
			return buf;
		}
		else
		{
			return val->m_str;
		}
	}
}

//------------------------------------------------------------------------------
const char* JsonValue::get( int index, const char* def, char* buf, int maxSize )
{
	JsonValue* val = 0;
	if ( m_type == JsonValue::JLIST )
	{
		val = m_values->get( index );
	}

	if ( !val )
	{
		if ( buf )
		{
			strncpy( buf, def, maxSize ? maxSize : 2000000000);
			return buf;
		}
		else
		{
			return def;
		}
	}
	else
	{
		if ( buf )
		{
			strncpy( buf, val->m_str, maxSize ? maxSize : 2000000000);
			return buf;
		}
		else
		{
			return val->m_str;
		}
	}
}

//------------------------------------------------------------------------------
int JsonValue::read( const char* buf )
{
	clear();
	m_valid = true;
	const char* start = buf;
	bool valueComplete = false;
	bool inList = false;
	bool inObject = false;
	int listIterator = 0;
	do
	{
		for( ; *buf && isspace(*buf); buf++ );

		if ( !(*buf) )
		{
			return 0;
		}

		switch( *buf )
		{
			case ']':
			{
				if ( !inList )
				{
					return 0;
				}
				buf++;
				valueComplete = true;
				addValues();
				m_values->invertLinkedList();
				break;
			}

			case '}':
			{
				if ( !inObject )
				{
					m_valid = false;
					return 0;
				}
				buf++;
				valueComplete = true;
				addValues();
				m_values->invertLinkedList();
				break;
			}

			case '[':
			{
				if ( inList || inObject )
				{
					m_valid = false;
					return 0;
				}
				inList = true;
				buf++;
				
				m_str.clear();
				m_type = JsonValue::JLIST;
				for( ; *buf && isspace(*buf); buf++ );

				if ( *buf == ']' )
				{
					addValues();
					valueComplete = true;
					break;
				}

				goto AddElement;
			}

			case '{':
			{
				if ( inList || inObject )
				{
					m_valid = false;
					return 0;
				}
				inObject = true;
				buf++;

				m_str.clear();
				m_type = JsonValue::JOBJECT;
				for( ; *buf && isspace(*buf); buf++ );

				if ( *buf == '}' )
				{
					addValues();
					valueComplete = true;
					break;
				}

				goto AddElement;
			}

			case ',':
			{
				buf++;
AddElement:
				if ( inList )
				{
					// expect the item
					addValues();
					JsonValue* val = m_values->add( listIterator++ );
					int bytes = val->read( buf );
					if ( !bytes )
					{
						m_valid = false;
						return 0;
					}
					buf += bytes;
				}
				else if ( inObject )
				{
					// expect the key
					int read = readRawValue(buf);
					m_type = JsonValue::JOBJECT; // clobber what readRawValue() filled in
					
					if ( !read )
					{
						return false;
					}
					buf += read;

					for( ; *buf && isspace(*buf); buf++ );
					if ( *buf != ':' )
					{
						return false;
					}
					buf++; // skip marker

					addValues();
					JsonValue* val = m_values->add( Hash::fnv64Str(m_str) );
					val->m_key = m_str;
					int bytes = val->read( buf );
					if ( !bytes )
					{
						m_valid = false;
						return 0;
					}
					buf += bytes;
				}
				else
				{
					m_valid = false;
					return 0;
				}
				
				break;
			}

			default:
			{
				int bytes = readRawValue( buf );
				if ( !bytes )
				{
					m_valid = false;
					return 0;
				}
				buf += bytes;

				for( ; *buf && isspace(*buf); buf++ );

				if ( *buf == ':' )
				{
					// I am a key, now get whatever I'm the key FOR
					buf++;
				}
				else
				{
					valueComplete = true;
				}

				break;
			}
		}

	} while( !valueComplete );

	m_valid = true;
	
	return (int)(buf - start);
}

//------------------------------------------------------------------------------
void JsonValue::write( Cstr& str )
{
	bool comma = false;
	switch( m_type )
	{
		case JsonValue::JLIST:
		{
			str += '[';
			addValues();
			int i = 0;
			for( JsonValue *V = m_values->get(i); V; V = m_values->get(i) )
			{
				if ( comma ) // only insert a comma for the 2nd through nth
				{
					str += ',';
				}
				comma = true;
				V->write( str );
				i++;
			}

			str += ']';
			break;
		}

		case JsonValue::JOBJECT:
		{
			str += '{';
			addValues();
			for( JsonValue *V = m_values->getFirst(); V; V = m_values->getNext() )
			{
				if ( comma ) // only insert a comma for the 2nd through nth
				{
					str += ',';
				}
				comma = true;
				str += '\"';
				UTF8toJsonUnicode( V->m_key, str );
				str += "\":";
				V->write( str );
			}

			str += '}';
			break;
		}

		case JsonValue::JSTRING:
		{
			str += '\"';
			UTF8toJsonUnicode( m_str, str ); // strings are just written out
			str += '\"';
			break;
		}

		default:
		{
			str += m_str;
			break;
		}
	}
}

//------------------------------------------------------------------------------
void JsonValue::writePretty( Cstr& str )
{
	if ( !m_valid )
	{
		return;
	}

	bool comma = false;
	switch( m_type )
	{
		case JsonValue::JLIST:
		{
			str += " [ ";
			addValues();
			int i =0;
			for( JsonValue *V = m_values->get(i); V; V = m_values->get(i) )
			{
				if ( comma ) // only insert a comma for the 2nd through nth
				{
					str += ",\n";
				}
				comma = true;

				// recurse
				V->writePretty( str );
				i++;
			}

			str += " ]\n";
			break;
		}

		case JsonValue::JOBJECT:
		{
			str += " { ";
			addValues();
			for( JsonValue *V = m_values->getFirst(); V; V = m_values->getNext() )
			{
				if ( comma ) // only insert a comma for the 2nd through nth
				{
					str += ",\n";
				}
				comma = true;

				str += '\"';
				UTF8toJsonUnicode( V->m_key, str );
				str += "\" : ";

				V->writePretty( str );
			}

			str += " }\n";
			break;
		}

		case JsonValue::JSTRING:
		{
			str += '\"';
			UTF8toJsonUnicode( m_str, str ); // strings are just written out
			str += '\"';
			break;
		}

		default:
		{
			str += m_str;
			break;
		}
	}
}

//------------------------------------------------------------------------------
int JsonValue::readRawValue( const char* buf )
{
	const char* start = buf;
	// expecting exactly and only a single value
	for( ; *buf && isspace(*buf); buf++ );

	m_str.clear();

	if ( *buf == '\"' )
	{
		buf++;
		int chars = strJsonUnicodeToUTF8( buf, m_str );
		if ( chars == -1 )
		{
			return -1;
		}

		buf += chars + 1; // skip string and closing quotes
		m_type = JsonValue::JSTRING;
	}
	else
	{
		int pos = 0;
		bool decimal = false;
		bool exp = false;
		for( ; *buf ; buf++, pos++ )
		{
			if ( *buf == '.' )
			{
				decimal = true;
			}
			if ( tolower(*buf) == 'e' )
			{
				exp = true;
			}

			// reach a terminator?
			if ( isspace(*buf) || *buf == ']' || *buf == '}' || *buf == ',' )
			{
				break;
			}

			m_str += *buf;

			if ( m_str.size() == 5 || m_str.size() == 4 )
			{
				if ( tolower(m_str[0]) == 't'
					 && tolower(m_str[1]) == 'r'
					 && tolower(m_str[2]) == 'u'
					 && tolower(m_str[3]) == 'e' )
				{
					m_type = JsonValue::JBOOL;
					m_int = 1;
					return (int)(buf - start) + 1;
				}
				else if ( tolower(m_str[0]) == 'f'
						  && tolower(m_str[1]) == 'a'
						  && tolower(m_str[2]) == 'l'
						  && tolower(m_str[3]) == 's'
						  && tolower(m_str[4]) == 'e' )
				{
					m_type = JsonValue::JBOOL;
					m_int = 0;
					return (int)(buf - start) + 1;
				}
				else if ( tolower(m_str[0]) == 'n'
						  && tolower(m_str[1]) == 'u'
						  && tolower(m_str[2]) == 'l'
						  && tolower(m_str[3]) == 'l' )
				{
					m_type = JsonValue::JNULL;
					return (int)(buf - start) + 1;
				}
			}
		}

		if ( decimal || exp )
		{
			m_type = JsonValue::JFLOAT;
			m_float = atof( m_str );
			m_int = (long long)m_float;
		}
		else
		{
			m_type = JsonValue::JINT;
#ifdef _WIN32
			m_int = _strtoi64( m_str, 0, 10);
#else
			m_int = strtoll( m_str, 0, 10);
#endif
			m_float = (double)m_int;
		}
	}

	return (int)(buf - start);
}

//------------------------------------------------------------------------------
JsonValue* JsonValue::checkAdd( const char* key )
{
	m_type = JsonValue::JOBJECT;
	long long keyval = Hash::fnv64Str(key);
	addValues();
	JsonValue* val = m_values->get( keyval );
	if ( !val )
	{
		val = m_values->add( keyval );
		val->m_key = key;
	}
	return val;
}

//------------------------------------------------------------------------------
JsonValue* JsonValue::checkAdd( const int index )
{
	m_type = JsonValue::JLIST;
	addValues();
	if ( index < 0 ) // adding a new blank
	{
		return m_values->add( m_index++ );
	}
	else if ( index < m_index ) // requesting an existing one
	{
		return m_values->get( index );
	}

	// make sure all the numbers inbetween are consumed
	for( int i=m_index; i<index; i++ )
	{
		m_values->add( i );
	}

	m_index = index;
	return m_values->add( m_index++ );
}

//------------------------------------------------------------------------------
int JsonValue::unicodeToUTF8( const unsigned int unicode, char* out, const unsigned int maxLen )
{
	if ( unicode <= 0x7f ) 
	{
		out[0] = (char)unicode;
		return 1;
	} 
	else if (unicode <= 0x7FF) 
	{
		if ( maxLen < 2 )
		{
			return -1;
		}

		
		out[0] = (char)(0xC0 | (0x1F & (unicode >> 6)));
		out[1] = (char)(0x80 | (0x3F & unicode));
		return 2;
	} 
	else if (unicode <= 0xFFFF) 
	{
		if ( maxLen < 3 )
		{
			return -1;
		}

		out[0] = (char)(0xE0 | (0x0F & (unicode >> 12)));
		out[1] = (char)(0x80 | (0x3F & (unicode >> 6)));
		out[2] = (char)(0x80 | (0x3F & unicode));
		return 3;
	}
	else if (unicode <= 0x10FFFF) 
	{
		if ( maxLen < 3 )
		{
			return -1;
		}
		
		out[0] = (char)(0xF0 | (0x07 & (unicode >> 18)));
		out[1] = (char)(0x80 | (0x3F & (unicode >> 12)));
		out[2] = (char)(0x80 | (0x3F & (unicode >> 6)));
		out[3] = (char)(0x80 | (0x3F & unicode));
		return 4;
	}

	return -1;
}

//------------------------------------------------------------------------------
JsonValue& JsonValue::object()
{
	m_type = JsonValue::JOBJECT;
	addValues();
	m_values->clear();
	return *this;
}

//------------------------------------------------------------------------------
JsonValue& JsonValue::list()
{
	m_type = JsonValue::JLIST;
	addValues();
	m_values->clear();
	return *this;
}

//------------------------------------------------------------------------------
void JsonValue::UTF8toJsonUnicode( const char* in, Cstr& str )
{
	for( unsigned int pos = 0; in[pos]; pos++ )
	{
		unsigned int unicodeCharacter = 0;
		if ( in[pos] & 0x80 ) // marker for multi-byte
		{
			if ( (in[pos] & 0x40) ) // 0x11_xxxxx
			{
				if ( (in[pos] & 0x20) ) // 0x111_xxxx
				{
					if ( (in[pos] & 0x10) ) // 0x1111_xxx
					{
						if ( (in[pos] & 0x08) ) // 0x11111_xx
						{
							if ( (in[pos] & 0x04) // 0x111111_x
								 && !(in[pos] & 0x02) // 0x1111110x?
								 && in[pos+1] && ((in[pos+1] & 0xC0) == 0x80)
								 && in[pos+2] && ((in[pos+2] & 0xC0) == 0x80)
								 && in[pos+3] && ((in[pos+3] & 0xC0) == 0x80)
								 && in[pos+4] && ((in[pos+4] & 0xC0) == 0x80)
								 && in[pos+5] && ((in[pos+5] & 0xC0) == 0x80) )
							{
								// 0x1111110x : 0x10000000 : 0x10000000 : 0x10000000 : 0x10000000 : 0x10000000
								unicodeCharacter = (((unsigned int)in[pos]) & 0x01) << 31
												   | (((unsigned int)in[pos+1]) & 0x3F) << 25
												   | (((unsigned int)in[pos+2]) & 0x3F) << 18
												   | (((unsigned int)in[pos+3]) & 0x3F) << 12
												   | (((unsigned int)in[pos+4]) & 0x3F) << 6
												   | (((unsigned int)in[pos+5]) & 0x3F);
							}
							// 0x111110xx:
							else if ( in[pos+1] && ((in[pos+1] & 0xC0) == 0x80)  
									  && in[pos+2] && ((in[pos+2] & 0xC0) == 0x80)
									  && in[pos+3] && ((in[pos+3] & 0xC0) == 0x80)
									  && in[pos+4] && ((in[pos+4] & 0xC0) == 0x80) )
							{
								// 0x111110xx : 0x10000000 : 0x10000000 : 0x10000000 : 0x10000000
								unicodeCharacter = (((unsigned int)in[pos]) & 0x03) << 25
												   | (((unsigned int)in[pos+1]) & 0x3F) << 18
												   | (((unsigned int)in[pos+2]) & 0x3F) << 12
												   | (((unsigned int)in[pos+3]) & 0x3F) << 6
												   | (((unsigned int)in[pos+4]) & 0x3F);
							}
						}
						// 0x11110xxx:
						else if ( in[pos+1] && ((in[pos+1] & 0xC0) == 0x80) 
								  && in[pos+2] && ((in[pos+2] & 0xC0) == 0x80)
								  && in[pos+3] && ((in[pos+3] & 0xC0) == 0x80) )
						{
							// 0x11110xxx : 0x10000000 : 0x10000000 : 0x10000000
							unicodeCharacter = (((unsigned int)in[pos]) & 0x07) << 18
											   | (((unsigned int)in[pos+1]) & 0x3F) << 12
											   | (((unsigned int)in[pos+2]) & 0x3F) << 6
											   | (((unsigned int)in[pos+3]) & 0x3F);
						}
					}
					// 0x1110xxxx:
					else if ( in[pos+1] && ((in[pos+1] & 0xC0) == 0x80)
							  && in[pos+2] && ((in[pos+2] & 0xC0) == 0x80) )
					{
						// 0x1110xxxx : 0x10000000 : 0x10000000
						unicodeCharacter = (((unsigned int)in[pos]) & 0x0F) << 12
										   | (((unsigned int)in[pos+1]) & 0x3F) << 6
										   | (((unsigned int)in[pos+2]) & 0x3F);
					}
				}
				// 0x110xxxxx:
				else if ( in[pos+1] && ((in[pos+1] & 0xC0) == 0x80) )
				{
					// 0x110xxxxx : 0x10000000
					unicodeCharacter = (((unsigned int)in[pos]) & 0x1F) << 6
									   | (((unsigned int)in[pos+1]) & 0x7F);
				}
			}

			if ( unicodeCharacter )
			{
				if ( (unicodeCharacter & 0x0000FFFF) != unicodeCharacter )
				{
					// TODO: figure out how to emit two UTF-16s based on the UTF-8 we
					// are decoding if it takes more than 16 bits;
					// "http://www.ietf.org/rfc/rfc4627.txt"
					//
					// To escape an extended character that is not in the Basic Multilingual
					// Plane, the character is represented as a twelve-character sequence,
					// encoding the UTF-16 surrogate pair.  So, for example, a string
					// containing only the G clef character (U+1D11E) may be represented as
					// "\uD834\uDD1E"
					//
					// ref: http://www.russellcottrell.com/greek/utilities/SurrogatePairCalculator.htm
				}

				str.appendFormat( "\\u%04X", unicodeCharacter & 0x0000FFFF );
			}
		}
		else
		{
			switch( in[pos] )
			{
				case '\"': str += '\\'; str += '\"'; break;
				case '\\': str += '\\'; str += '\\'; break;
				case '\b': str += '\\'; str += 'b'; break;
				case '\f': str += '\\'; str += 'f'; break;
				case '\n': str += '\\'; str += 'n'; break;
				case '\r': str += '\\'; str += 'r'; break;
				case '\t': str += '\\'; str += 't'; break;
				case '/': str += '\\'; str += '/'; break;
				default: str += in[pos]; break;
			}
		}
	}
}

//------------------------------------------------------------------------------
int JsonValue::strJsonUnicodeToUTF8( const char* in, Cstr& out )
{
	int pos = 0;
	for( int pos=0; in[pos]; pos++ )
	{
		if ( in[pos] != '\\' )
		{
			if ( in[pos] == '\"' )
			{
				return pos;
			}

			out += in[pos];
		}
		else
		{
			pos++;
			switch( in[pos] )
			{
				case 0:
				{
					return pos;
				}

				case '\"':
				case '\\':
				case '/':
				{
					out += in[pos];
					break;
				}

				case 'b':
				{
					out += '\b';
					break;
				}

				case 't':
				{
					out += '\t';
					break;
				}

				case 'n':
				{
					out += '\n';
					break;
				}

				case 'r':
				{
					out += '\r';
					break;
				}

				case 'f':
				{
					out += '\f';
					break;
				}

				case 'u':
				{
					// pull out the first HEX code
					unsigned int u = 0;
					pos++;
					for ( int i=0; i<4; pos++, i++ )
					{
						if ( !in[pos] )
						{
							return -1;
						}

						char c = in[pos];
						if ( c >= '0'  &&  c <= '9' )
						{
							u = (u << 4) + (c - '0');
						}
						else if ( c >= 'a'  &&  c <= 'f' )
						{
							u = (u << 4) + ((c - 'a') + 10);
						}
						else if ( c >= 'A'  &&  c <= 'F' )
						{
							u = (u << 4) + ((c - 'A') + 10);
						}
						else
						{
							return -1;
						}
					}

					// if this one indicates it is part of a surrogate
					// pair, pull the next one and mix it
					if ( (u >= 0xD800) && (u <= 0xDBFF) )
					{
						if ( (in[pos] != '\\') ||(in[pos+1] != 'u') )
						{
							return -1;
						}
						pos += 2;

						unsigned int p = 0;
						for ( int i=0; i<4; pos++, i++ )
						{
							if ( !in[pos] )
							{
								return -1;
							}

							char c = in[pos];
							if ( c >= '0'  &&  c <= '9' )
							{
								p = (p << 4) + (c - '0');
							}
							else if ( c >= 'a'  &&  c <= 'f' )
							{
								p = (p << 4) + ((c - 'a') + 10);
							}
							else if ( c >= 'A'  &&  c <= 'F' )
							{
								p = (p << 4) + ((c - 'A') + 10);
							}
							else
							{
								return -1;
							}
						}
				
						u = 0x10000 + ((u & 0x3FF) << 10) + (p & 0x3FF);
					}
					
					char str[8];
					int written = unicodeToUTF8( u, str, 8 );
					if ( written == -1 )
					{
						return -1;
					}
					out.append( str, written );
					break;
				}
			}
		}
	}

	return pos;
}


#endif


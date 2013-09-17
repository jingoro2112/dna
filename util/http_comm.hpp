#ifndef HTTP_COMM_HPP
#define HTTP_COMM_HPP
/*------------------------------------------------------------------------------*/

#include "tcp_hdlr.hpp"
#include "str.hpp"

namespace HTTP
{

bool fetchDocument( Cstr &doc, const char* host, const char* target, int port =80 );

}

#endif

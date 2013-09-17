#include "http_comm.hpp"
#include "str.hpp"

#include <stdlib.h>

namespace HTTP
{
	
//------------------------------------------------------------------------------
bool fetchDocument( Cstr &doc, const char* host, const char* target, int port )
{
	if ( !host || !target )
	{
		return false;
	}
	
	Cstr request;
	request.format( "GET http://%s/%s HTTP/1.1\r\n"
					"Host: %s\r\n"
					"\r\n",
					host, target, host );

	CTCP session;
	if ( !session.clientSocket(host, port) )
	{
		doc = "<1>";
		return false;
	}

	if ( !session.write(request, request.size()) )
	{
		doc = "<2>";
		return false;
	}

	int retBytes = 0;
	int totalBytes = 0;

	char *crcr = 0;
	// read header

	char buf[16000];
	while( (crcr == 0) && session.read(buf, 16000 - totalBytes, &retBytes) )
	{
		totalBytes += retBytes;
		if ( totalBytes >= 16000 )
		{
			doc.format( "header[%d] too big", totalBytes );
			return false;
		}
		crcr = strstr( buf, "\r\n\r\n" );
	}

	char *cl = strstr( buf, "Content-Length:" );
	if ( !cl )
	{
		doc.format( "<3> [%s]\n", buf );
		return false;
	}

	int expected = atoi( cl + 16 );
	if ( !expected )
	{
		doc = "<4>";
		return false;
	}

	int bodyBytes = totalBytes - ((crcr + 4) - buf);

	doc.clear();
	doc.append( crcr + 4, bodyBytes );

	while ( bodyBytes < expected )
	{
		if ( !session.read(buf, (expected > 16000) ? 16000 : (expected - bodyBytes), &retBytes) )
		{
			doc = "read failed";
			return false;
		}

		doc.append( buf, retBytes );
		bodyBytes += retBytes;
	}

	return true;
}

}

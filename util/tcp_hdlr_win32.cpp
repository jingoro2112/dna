#include "tcp_hdlr.hpp"
#pragma comment(lib, "ws2_32.lib")

//------------------------------------------------------------------------------
CTCP::CTCP( SOCKET fd /*=-1*/ )
{
	WORD wVersionRequested;
	WSADATA wsaData;
	wVersionRequested = MAKEWORD( 2, 2 );
	WSAStartup( wVersionRequested, &wsaData );
	m_fd = fd;
}

//------------------------------------------------------------------------------
CTCP::~CTCP()
{
	close();
}

//------------------------------------------------------------------------------
void CTCP::handle( SOCKET fd )
{
	close();
	m_fd = fd;
}

//------------------------------------------------------------------------------
bool CTCP::read( char *buf, int bytesToRead, int *bytesRead /*=0*/, int minBytesToRead /*=0*/, int milli_timeout /*=0*/ )
{
	if ( (m_fd <= 0) || (bytesToRead <= 0) )
	{
		return 0;
	}

	if ( milli_timeout )
	{
		if ( readable( milli_timeout ) <= 0 )
		{
			return false;
		}
	}

	int pos = 0;
	int recieved = 0;
	do
	{
		recieved = recv( m_fd, buf + pos, bytesToRead - pos, 0 ); // ask for a full page

		if ( recieved <= 0 )
		{
			if ( bytesRead )
			{
				*bytesRead = 0;
			}
			close();
			return false;
		}

		pos += recieved;

	} while( pos < minBytesToRead );

	if ( bytesRead )
	{
		*bytesRead = pos;
	}

	return true;
}

//------------------------------------------------------------------------------
bool CTCP::write( const char *buf, const int size )
{
	int bytesWritten = 0;
	do
	{
		int bytes = send( m_fd, buf + bytesWritten, size - bytesWritten, 0 );
		if ( bytes == SOCKET_ERROR )
		{
			return false;
		}
		bytesWritten += bytes;
		
	} while( bytesWritten < size );
	return true;
}

//------------------------------------------------------------------------------
bool CTCP::serverSocket( unsigned short port_number,
						 bool doListen /*=true*/,
						 int ListenQueue /*=128*/ )
{
	sockaddr_in location;
	memset( &location, 0, sizeof(location) );

	location.sin_family = AF_INET;
	location.sin_addr.s_addr = htonl(INADDR_ANY);
	location.sin_port = htons(port_number);

	if ( (m_fd = ::socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
	{
		close();
		return false;
	}

	if ( !makeReusable(m_fd) )
	{
		close();
		return false;
	}	

	if ( ::bind(m_fd, reinterpret_cast<sockaddr *>(&location), sizeof(location)) == -1)
	{
		close();
		return false;
	}

	if ( doListen && ::listen(m_fd, ListenQueue) == -1)
	{
		close();
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------
bool CTCP::clientSocket( const char *address,
						 unsigned short port_number,
						 bool keepAlive /*=true*/,
						 bool noNagle /*=false*/ )
{
	if ( !address || !address[0] )
	{
		return false;
	}

	struct hostent* answer = gethostbyname( address );

	if ( !answer )
	{
		return false;
	}

	sockaddr_in location;
	memset(&location, 0, sizeof(location));

	location.sin_family = AF_INET;
	location.sin_addr = **(struct in_addr **)(answer->h_addr_list);
	location.sin_port = htons(port_number);

	if ( (m_fd = ::socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
	{
		m_fd = 0;
		return 0;
	}

	if ( ::connect( m_fd, reinterpret_cast<sockaddr *>(&location), sizeof(location)) == -1 )
	{
		close();
		return false;
	}

	if ( (keepAlive && !enableKeepAlive(m_fd)) || (noNagle && !disableNagleAlgorithm(m_fd)) )
	{
		close();
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------
SOCKET CTCP::accept( sockaddr *info,
					 int *length,
					 bool keepAlive /*=true*/,
					 unsigned int millisecondTimeout /*=0*/,
					 bool noNagle /*=false*/ )
{
	if ( !length || !info )
	{
		return 0;
	}

	*length = sizeof( sockaddr );

	fd_set accepter;
	timeval timeout;
	timeout.tv_sec = 1;
	timeout.tv_usec = millisecondTimeout * 1000;

	FD_ZERO( &accepter );
	FD_SET( m_fd, &accepter );
	int ret = select( 0, &accepter, 0, 0, &timeout );
	if ( ret == 0 )
	{
		return INVALID_SOCKET;
	}
	
	SOCKET new_fd = ::accept( m_fd, info, length );
	if ( new_fd != INVALID_SOCKET )
	{
		if ( !makeReusable(new_fd) )
		{
			closesocket( new_fd );
			return INVALID_SOCKET;
		}
		
		if ( (keepAlive && !enableKeepAlive(new_fd)) || (noNagle && !disableNagleAlgorithm(new_fd)) )
		{
			closesocket( new_fd );
			return INVALID_SOCKET;
		}
	}
	
	return new_fd;
}

//------------------------------------------------------------------------------
bool CTCP::isBound( unsigned short port_number, SOCKET fd )
{
	sockaddr_in info;
	int size = sizeof(info);
	if (::getsockname( fd, reinterpret_cast<sockaddr *>(&info), &size ))
	{
		return false;
	}

	return (info.sin_port == htons(port_number));
}

//------------------------------------------------------------------------------
bool CTCP::enableKeepAlive( SOCKET fd )
{
	bool optval = 1;
	if ( ::setsockopt( fd, SOL_SOCKET, SO_KEEPALIVE, (char *)&optval, sizeof(bool)) == SOCKET_ERROR )
	{
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------
bool CTCP::disableNagleAlgorithm( SOCKET fd )
{
	// Make file descriptor use TCP_NODELAY
	bool optval = 1;
	if ( ::setsockopt(fd, SOL_SOCKET, TCP_NODELAY, (char *)&optval, sizeof(bool)) == SOCKET_ERROR )
	{
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------
bool CTCP::setSendBufferSize( int bufsize, SOCKET fd )
{
	if (bufsize <= 0) { return false; }
	if (bufsize > 65536) { bufsize = 65536; }

	// Adjust file descriptor's send buffer
	if ( ::setsockopt( fd, SOL_SOCKET, SO_SNDBUF, (char *)&bufsize, sizeof(bufsize)) == SOCKET_ERROR )
	{
		return false;
	}

	return true;
}


//------------------------------------------------------------------------------
bool CTCP::setReceiveBufferSize( int bufsize, SOCKET fd )
{
	if (bufsize <= 0) { return false; }
	if (bufsize > 65536) { bufsize = 65536; }

	// Adjust file descriptor's receive buffer
	if ( ::setsockopt( fd, SOL_SOCKET, SO_RCVBUF, (char *)&bufsize, sizeof(bufsize)) == SOCKET_ERROR )
	{
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------
bool CTCP::makeReusable( SOCKET fd )
{
	bool optval = 1;

	if ( ::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&optval, sizeof(bool)) == SOCKET_ERROR )
	{
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------
bool CTCP::makeNonBlocking( SOCKET fd )
{
	unsigned long noblock = 1;
	int err = ioctlsocket( fd, FIONBIO, &noblock );

	return err == 0;
}

//------------------------------------------------------------------------------
int CTCP::bytesWaiting( SOCKET fd )
{
	int retval = recv( fd, 0, 2000000000, MSG_PEEK );
	return retval == SOCKET_ERROR ? -1 : retval;
}

//------------------------------------------------------------------------------
int CTCP::resultRead( char *buf,
						  unsigned int length,
						  unsigned int *result )
{
	return  ceReadError;
}

//------------------------------------------------------------------------------
void CTCP::close()
{
	if ( m_fd > 0 )
	{
		closesocket( m_fd );
	}

	m_fd = INVALID_SOCKET;
}

//------------------------------------------------------------------------------
int CTCP::readable( int arg_timeout )
{
	return 0;
}

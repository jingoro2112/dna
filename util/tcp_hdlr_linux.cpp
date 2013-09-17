#include "tcp_hdlr.hpp"

//------------------------------------------------------------------------------
CTCP::CTCP( int fd /*=-1*/ )
{
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
bool CTCP::read( char *buf, int bytesToRead, int *bytesRead /*=0*/, int minBytesToRead /*=0*/, int milliTimeout /*=0*/ )
{
	if ( (m_fd <= 0) || (bytesToRead <= 0) )
	{
		return false;
	}

	if ( milliTimeout )
	{
		if ( readable( milliTimeout ) <= 0 )
		{
			return false;
		}
	}
	
	int pos = 0;
	int recieved = 0;
	do
	{
		recieved = fdRead( m_fd, buf + pos, bytesToRead - pos ); // ask for a full page

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
		
	} while( recieved < minBytesToRead );

	if ( bytesRead )
	{
		*bytesRead = pos;
	}

	return true;
}

//------------------------------------------------------------------------------
bool CTCP::write( const char *buf, const int size )
{
	return fdWrite( m_fd, buf, size ) != -1;
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

	struct addrinfo *res;
	if ( getaddrinfo(address, 0, 0, &res) != 0 )
	{
		return false;
	}

	sockaddr_in location;
	memset( &location, 0, sizeof(location) );

	location.sin_family = AF_INET;
	location.sin_addr = ((sockaddr_in *)res->ai_addr)->sin_addr;
	location.sin_port = htons(port_number);

	if ( (m_fd = ::socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
	{
		m_fd = -1;
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
int CTCP::accept( sockaddr *info,
				  int *length,
				  bool keepAlive /*=true*/,
				  unsigned int millisecondTimeout /*=0*/,
				  bool noNagle /*=false*/ )
{
	if ( !length || !info )
	{
		return -1;
	}

	// We call "readable" first because Solaris has a bug where
	// threads will deadlock if ::accept() and ::close() are called at
	// the same time.  This way, we are blocking in ::poll().
	// Also, it is a good idea to ::poll() before going to ::accept().
	if ( millisecondTimeout > 0 )
	{
		fd_set accepter;
		timeval timeout;
		timeout.tv_sec = 1;
		timeout.tv_usec = millisecondTimeout * 1000;

		FD_ZERO( &accepter );
		FD_SET( fd, &accepter );
		int ret = select( 0, &accepter, 0, 0, &timeout );
		if ( ret == 0 )
		{
			return -1;
		}
	}

	// Otherwise, we've got something to accept.

	int new_fd;
	*length = sizeof(sockaddr);

	while ( (new_fd = ::accept(m_fd, info, (unsigned int *)length)) == -1)
	{
		if ( errno == EWOULDBLOCK )
		{
			return -1;
		}
		
		if ( errno == EINTR || errno == EAGAIN )
		{
			continue;
		}

		return -1;
	}

//	inet_ntoa( ((sockaddr_in *)info)->sin_addr )

	if ( !makeReusable(m_fd) )
	{
		::close( new_fd );
		return -1;
	}

	if ( (keepAlive && !enableKeepAlive(new_fd)) || (noNagle && !disableNagleAlgorithm(new_fd)) )
	{
		::close( new_fd );
		return -1;
	}

	return new_fd;
}

//------------------------------------------------------------------------------
bool CTCP::isBound( unsigned short port_number, SOCKET fd )
{
	sockaddr_in info;
	int size = sizeof(info);
	if (::getsockname( fd, reinterpret_cast<sockaddr *>(&info), reinterpret_cast<unsigned int *>(&size) ))
	{
		return false;
	}

	return (info.sin_port == htons(port_number));
}

//------------------------------------------------------------------------------
int CTCP::portFromInfo( const sockaddr& info )
{
	return ((sockaddr_in *)&info)->sin_port;
}

//------------------------------------------------------------------------------
char* CTCP::ipFromInfo( const sockaddr& info )
{
	return inet_ntoa( ((sockaddr_in *)&info)->sin_addr );
}

//------------------------------------------------------------------------------
bool CTCP::enableKeepAlive( int fd )
{
	int optval = 1;
	if ( ::setsockopt( fd, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval)) == -1 )
	{
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------
bool CTCP::disableNagleAlgorithm( int fd )
{
	// Make file descriptor use TCP_NODELAY
	int optval = 1;
	if ( ::setsockopt( fd, SOL_SOCKET, TCP_NODELAY, reinterpret_cast<const char *>(&optval), sizeof(optval)) == -1)
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
	if ( ::setsockopt( fd, SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof(bufsize)) == -1 )
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
	if ( ::setsockopt( fd, SOL_SOCKET, SO_RCVBUF, &bufsize, sizeof(bufsize)) == -1 )
	{
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------
bool CTCP::makeReusable( SOCKET fd )
{
	int optval = 1;

	if ( ::setsockopt( fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1 )
	{
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------
bool CTCP::makeNonBlocking( SOCKET fd )
{
	int status = -1;
	if( (status = ::fcntl( fd, F_GETFL, 0)) >= 0 )
	{
		status |= O_NONBLOCK;

		if ( ::fcntl( fd, F_SETFL, status) != -1 )
		{
			return true;
		}
	}
	return false;
}

//------------------------------------------------------------------------------
int CTCP::bytesWaiting( SOCKET fd )
{
	int retval = 0;

	short poll_events = POLLIN;

	pollfd readfd;
	readfd.fd = fd;
	readfd.events = poll_events;
	readfd.revents = 0;
	retval = ::poll(&readfd, 1, 0);

	return retval;
}

//------------------------------------------------------------------------------
int CTCP::resultRead( char *buf,
					  unsigned int length,
					  unsigned int *result )
{
	if ( (m_fd <= 0) || !buf || !length)
	{
		if ( result )
		{
			*result = ceReadError;
		}

		return -1;
	}

	int retval = fdRead( m_fd, buf, length );

	if ( retval > 0 )
	{
		if ( result )
		{
			*result = ceReadSuccess;
		}
	}
	else if ( retval == 0 )
	{
		if ( result )
		{
			*result = ceReadEOF;
		}
	}
	else if ( result )
	{
		*result = ceReadError;
	}

	return retval;
}

//------------------------------------------------------------------------------
void CTCP::close()
{
	if ( m_fd > 0 )
	{
		::close( m_fd );
	}

	m_fd = -1;
}

//------------------------------------------------------------------------------
int CTCP::readable( int arg_timeout )
{
	short poll_events = POLLIN;

	pollfd readfd;
	readfd.fd = m_fd;
	readfd.events = poll_events;
	readfd.revents = 0;

	// Loop to handle EINTR and EAGAIN on error
	int result = 0;
	for(;;)
	{
		if (arg_timeout)
		{
			result = ::poll( &readfd, 1, arg_timeout );
		}
		else
		{
			result = ::poll( &readfd, 1, -1 );
		}

		// Break on data, timeout, or error
		if (result > 0 || (arg_timeout && result == 0) ||
			  (result < 0 && errno != EINTR && errno != EAGAIN))
		{
			break;
		}

		// Reset the arguments to poll
		readfd.fd = m_fd;
		readfd.events = poll_events;
		readfd.revents = 0;
	}

	// It timed out!
	if ( result == 0 )
	{
		return 0;
	}

	if ( result < 0 )
	{
		return -1;
	}

	// We better have only 1 ready... or else something is really weird!
	if ( result != 1 )
	{
		return -1;
	}

	// First, check for read() then errors.
	if ( readfd.revents & poll_events )
	{
		return 1;
	}
	else if ( readfd.revents & POLLHUP )
	{
		return -2; // Closed
	}
	else
	{
		return -1;
	}
}

//------------------------------------------------------------------------------
int CTCP::fdRead( const int fd, char *buf, const unsigned int len )
{
	if ( !buf || !len )
	{
		return 0;
	}

	// Loop on EINTR, EAGAIN, and ETIMEDOUT
	int bytes = 0;
	do
	{
		bytes = ::read( fd, buf, len );

	} while ( bytes < 0 && (errno == EINTR || errno == EAGAIN || errno == ETIMEDOUT));

	return bytes;
}

//------------------------------------------------------------------------------
int CTCP::fdWrite( const int fd, const char *buf, const unsigned int len )
{
	if ( !buf )
	{
		return 0;
	}

	// Handle the 0 bytes case
	if ( !len )
	{
		return ::write( fd, buf, len );
	}

	unsigned int remaining = len;

	while ( remaining )
	{
		int bytes = ::write( fd, buf, remaining );

		if ( bytes <= 0 )
		{
			if (errno == EINTR || errno == EAGAIN || errno == ETIMEDOUT)
			{
				continue;
			}

			return -1;
		}

		remaining -= bytes;
		buf += bytes;
	}

	return len;
}

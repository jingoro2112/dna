#ifndef TCP_HDLR_HPP
#define TCP_HDLR_HPP
/*----------------------------------------------------------------------------*/

#ifdef WIN32
#include <winsock2.h>
#else
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/io.h>
#include <unistd.h>
#include <netdb.h>
#include <signal.h>
#include <errno.h>
#include <poll.h>
#define SOCKET int
#endif

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

//------------------------------------------------------------------------------
enum EReadResult
{
	ceReadSuccess,
	ceReadError,
	ceReadEOF,
	ceReadTimeout,

	ceReadLast
};

//------------------------------------------------------------------------------
// manage the connection and do coarse reads, this is a middle-class,
// primarily for reading a command stream FAST. The whole point of this
// class is to minimze dipping into the kernel. This class is NOT
// thread-safe.
class CTCP
{
public:
	CTCP( SOCKET fd =-1 );
	~CTCP();

	void handle( SOCKET fd );
	SOCKET takeOwnership() { SOCKET ret = m_fd; m_fd = 0; return ret; }
	SOCKET getDescriptor() const { return m_fd; }
	
	bool read( char *buf, int bytesToRead, int *bytesRead =0, int minBytesToRead =0, int milliTimeout =0 );
	bool write( const char *buf, const int size );

	// Takes a file descriptor as an argument and poll()'s waiting for
	// data for milliTimeout milliseconds (0 means forever). After
	// receiving some data, it does a single read() call and returns the
	// results.  Returns: # of bytes read. 0 on a close or timeout with
	// nothing to read -1 on errors.  The second version returns
	// a more precise indication of the result of the read.
	inline int timeoutRead( char *buf,
							unsigned int length,
							int milliTimeout,
							unsigned int *result =0 );

	// This is the same as the previous timeoutRead(), except that
	//  it avoids the initial poll(), it assumes the fd is ready for a
	//  blocking read(). In short, this is like read() except
	//  with timeoutRead() return semantics.
	int resultRead( char *buf,
					unsigned int length,
					unsigned int *result =0 );


	// make this instance a server socket
	bool serverSocket( unsigned short port_number,
					   bool doListen = true,
					   int ListenQueue = 128 );

	// make this instance a client socket and go connect it
	bool clientSocket( const char *address,
					   unsigned short port_number,
					   bool keepAlive = true,
					   bool noNagle = true );

	// returns 0 on error
	SOCKET accept( sockaddr *info,
				   int *length,
				   bool keepAlive = true,
				   unsigned int millisecondTimeout = 0,
				   bool noNagle = true );

	static int portFromInfo( const sockaddr& info );
	static char* ipFromInfo( const sockaddr& info );
	static bool isBound( unsigned short port_number, SOCKET fd );
	static bool enableKeepAlive( SOCKET fd );
	static bool disableNagleAlgorithm( SOCKET fd );
	static bool setSendBufferSize( int bufsize, SOCKET fd );
	static bool setReceiveBufferSize( int bufsize, SOCKET fd );
	static bool makeReusable( SOCKET fd );
	static bool makeNonBlocking( SOCKET fd );
	static int bytesWaiting( SOCKET fd ); // returns -1 on error

	void close();

private:

	// Return -2, -1, 0, 1, indicating a close, error, timeout,
	//  and readability of the argument descriptor over the time interval.
	//  An arg_timeout of 0 means block forever.
	int readable( int arg_timeout );

	static int fdRead( const int fd, char *buf, const unsigned int len );
	static int fdWrite( const int fd, const char *buf, const unsigned int len );

	SOCKET m_fd;
};

//------------------------------------------------------------------------------
int CTCP::timeoutRead( char *buf,
						   unsigned int length,
						   int arg_timeout,
						   unsigned int *result /*=0*/ )
{		
	if ( (m_fd <= 0) || !buf || !length )
	{
		if ( result )
		{
			*result = ceReadError;
		}

		return -1;
	}

	int retval = readable( arg_timeout );

	if ( retval == 1 )
	{
		retval = resultRead( buf, length, result );
	}
	else if ( retval == 0 )
	{
		if ( result )
		{
			*result = ceReadTimeout;
		}
	}
	else if (retval == -2)
	{
		if ( result )
		{
			*result = ceReadEOF;
		}

		retval = 0;
	}
	else if ( result )
	{
		*result = ceReadError;
	}

	return retval;
}

#endif

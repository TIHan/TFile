/*
Copyright (c) 2013, William F. Smith (TIHan)
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "tfile_server.h"

#include "tfile_shared.h"
#include "tinycthread.h"


static _bool server_initialized = _false;

/* Used in threads, TODO: Fix this. */
static _bool server_running = _false;
static SOCKET server_socket = INVALID_SOCKET;
static SOCKET server_socket6 = INVALID_SOCKET;
static thrd_t server_thread;


/*
====================
CreateServerHints
====================
*/
static struct addrinfo CreateServerHints( const int family, const int socketType, const int flags ) {
	// We can only create sockets for ipv4 and ipv6.
	if ( family != AF_INET && family != AF_INET6 ) {
		T_FatalError( "CreateServerHints: Bad socket family type" );
	}

	return T_CreateHints( family, socketType, flags );
}


/*
====================
CreateServer

TODO: Break out normal socket errors.
====================
*/
static _bool CreateServer( const int family, const int port, SOCKET *const socket ) {
	const struct addrinfo hints = CreateServerHints( family, SOCK_STREAM, AI_PASSIVE );

	struct addrinfo defaultInfo = T_CreateAddressInfo();
	struct addrinfo *result = &defaultInfo;
	char portStr[MAX_PORT_SIZE];

	T_itoa( port, portStr, MAX_PORT_SIZE );

	// Get address info.
	if ( getaddrinfo( 0, portStr, &hints, &result ) == SOCKET_ERROR ) {
		TFile_CleanupFailedSocket( "CreateServer: Unable to get address information.\n", INVALID_SOCKET, result );
		return _false;
	}

	// Attempt to create a socket.
	*socket = T_CreateSocket( family, result );
	if ( *socket == INVALID_SOCKET ) {
		TFile_CleanupFailedSocket( "CreateServer: Unable to create socket.\n", INVALID_SOCKET, result );
		return _false;
	}

	// Get file server info.
	if ( bind( *socket, result->ai_addr, result->ai_addrlen ) == SOCKET_ERROR ) {
		TFile_CleanupFailedSocket( "CreateServer: Unable to bind socket.\n", *socket, result );
		return _false;
	}

	// Set socket to non-blocking.
	if ( T_SocketNonBlocking( *socket ) == SOCKET_ERROR ) {
		TFile_CleanupFailedSocket( "CreateServer: Unable to set socket to non-blocking.\n", *socket, result );
		return _false;
	}

	// Set socket to reuse address.
	if ( T_SocketReuseAddress( *socket ) == SOCKET_ERROR ) {
		TFile_CleanupFailedSocket( "CreateServer: Unable to set socket to reuse address.\n", *socket, result );
		return _false;
	}

	// Free up what was allocated from getaddrinfo.
	freeaddrinfo( result );
	return _true;
}


/*
====================
TFile_ShutdownServer
====================
*/
void TFile_ShutdownServer( void ) {
	server_initialized = _false;
	server_running = _false;
	if ( closesocket( server_socket ) == SOCKET_ERROR ) {
		T_Error( "TFile_ShutdownServer: Unable to close file server socket.\n" );
		return;
	}
	T_Print( "File server shutdown.\n" );
}


/*
====================
TFile_InitServer
====================
*/
int TFile_InitServer( const int port ) {
	if ( server_initialized ) {
		T_FatalError( "TFile_InitServer: Server is already initialized" );
	}
	if ( !CreateServer( AF_INET, port, &server_socket ) || !CreateServer( AF_INET6, port, &server_socket6 ) ) {
		TFile_CleanupFailedSocket( NULL, server_socket, NULL ); // Clean up IPv4 socket in case only the IPv6 socket failed.
		T_Error( "TFile_InitServer: Unable to initialize server.\n" );
		return _false;
	}
	server_initialized = _true;
	T_Print( "File server initialized.\n" );
	return _true;
}


/*
====================
ServerThread

TODO: This could use some cleaning up.
TODO: Break out normal socket errors.
====================
*/
static int ServerThread( void *arg ) {
#define MAX_CONNECTIONS 256
#define MAX_SOCKETS MAX_CONNECTIONS + 2
#define SELECT_TIMEOUT 100000


	const SOCKET server = server_socket; // fix me
	const SOCKET server6 = server_socket6; // fix me

	SOCKET connections[MAX_CONNECTIONS] = { ZERO_SOCKET };
	int connectionCount = 0, addrLen = 0;
	_time_t baseTime = 0;
	_bool timeInitialized = _false;
	struct sockaddr_storage addr;
	_byte buffer[MAX_PACKET_SIZE];
	_time_t serverTime;

	if ( listen( server, 8 ) == SOCKET_ERROR || listen( server6, 8 ) == SOCKET_ERROR ) {
		T_Error( "ServerThread: Failed to listen on file server socket." );
		return 0;
	}

	addrLen = sizeof( addr );
	server_running = _true; // fix me
	while( 1 ) {
		SOCKET sockets[MAX_SOCKETS] = { ZERO_SOCKET };
		SOCKET reads[MAX_SOCKETS] = { ZERO_SOCKET };
		int i;

		serverTime = T_Milliseconds( &baseTime, ( int * )&timeInitialized );

		sockets[0] = server;
		sockets[1] = server6;

		for ( i = 2; i < MAX_SOCKETS; ++i ) {
			sockets[i] = connections[i - 2];
		}

		// Synchronous event demultiplexer.
		// It's ok that we are using select as its portable.
		// It may not be the fastest, but it's definitely quick enough for what we are trying to accomplish.
		if ( T_Select( sockets, MAX_SOCKETS, SELECT_TIMEOUT, reads ) == SOCKET_ERROR ) {
			continue;
		}

		for( i = 0; i < MAX_SOCKETS; ++i ) {
			if ( reads[i] == ZERO_SOCKET )
				continue;

			// Accept connections on IPv4 and IPv6.
			if ( reads[i] == server ) {
				if ( ( connections[connectionCount] = accept( server, ( struct sockaddr * )&addr,  &addrLen ) ) != SOCKET_ERROR ) {
					++connectionCount;
					// TODO
					T_Print( "Client connected.\n" );
				}
			} else if ( reads[i] == server6 ) {
				if ( ( connections[connectionCount] = accept( server6, ( struct sockaddr * )&addr,  &addrLen ) ) != SOCKET_ERROR ) {
					++connectionCount;
					// TODO
					T_Print( "Client connected.\n" );
				}
			} else {
				// Handle messages from the accepted connections.
				int bytes = recv( reads[i], ( char * )buffer, MAX_PACKET_SIZE, 0 );
				if ( bytes > 0 ) {
					// TODO
					T_Print( "Server received %i bytes.\n", bytes );
				}
			}
		}
	}
	return _true;
}


/*
====================
TFile_StartServer
====================
*/
void TFile_StartServer( void ) {
	if ( !server_initialized ) {
		T_FatalError( "TFile_StartServer: Server is not initialized" );
	}

	if ( server_running ) {
		T_FatalError( "TFile_StartServer: Server is already running" );
	}

	if ( thrd_create( &server_thread, ServerThread, NULL ) != thrd_success ) {
		T_FatalError( "TFile_StartServer: Unable to create thread" );
	}
}

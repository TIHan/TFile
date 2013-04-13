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

typedef struct fileServerInfo_s {
	struct sockaddr ai_addr;
	socklen_t ai_addrlen;
} fileServerInfo_t;


static tboolean file_server_initialized = tfalse;

/* Used in threads, TODO: Fix this. */
static tboolean file_server_running = tfalse;
static SOCKET file_server_socket;
static SOCKET file_server_socket6;
static thrd_t file_server_thread;


/*
====================
CreateHints
====================
*/
static struct addrinfo CreateFileServerHints( const int family, const int socketType, const int flags ) {
	struct addrinfo hints = T_CreateAddressInfo();

	// We can only create file servers for ipv4 and ipv6.
	if ( family != AF_INET && family != AF_INET6 ) {
		T_FatalError( "CreateFileServerHints: Bad socket family type" );
	}

	hints.ai_family = family;
	hints.ai_socktype = socketType;
	hints.ai_flags = flags;
	return hints;
}


/*
====================
GetFileServerStorage
====================
*/
static fileServerInfo_t GetFileServerInfo( const int family, const struct addrinfo *const info ) {
	fileServerInfo_t fileServerInfo;

	if ( !info ) {
		T_FatalError( "GetFileServerInfo: Unable to retrieve socket address" );
	}

	if ( info->ai_family == family ) {
		fileServerInfo.ai_addr = *info->ai_addr;
		fileServerInfo.ai_addrlen = info->ai_addrlen;
		return fileServerInfo;
	}
	return GetFileServerInfo( family, info->ai_next );
}


/*
====================
CreateFileServer
====================
*/
static tboolean CreateFileServer( const int family, const char *const port, SOCKET *const socket ) {
	const struct addrinfo hints = CreateFileServerHints( family, SOCK_STREAM, AI_PASSIVE );

	struct addrinfo defaultInfo = T_CreateAddressInfo();
	struct addrinfo *result = &defaultInfo;

	// Get address info.
	if ( getaddrinfo( 0, port, &hints, &result ) == SOCKET_ERROR ) {
		TFile_CleanupFailedSocket( "CreateFileServer: Unable to get address information.\n", INVALID_SOCKET, result );
		return tfalse;
	}

	// Attempt to create a socket.
	*socket = T_CreateSocket( family, result );
	if ( *socket == INVALID_SOCKET ) {
		TFile_CleanupFailedSocket( "CreateFileServer: Unable to create socket.\n", INVALID_SOCKET, result );
		return tfalse;
	}

	// Get file server info.
	if ( bind( *socket, result->ai_addr, result->ai_addrlen ) == SOCKET_ERROR ) {
		TFile_CleanupFailedSocket( "CreateFileServer: Unable to bind socket.\n", *socket, result );
		return tfalse;
	}

	// Set socket to non-blocking.
	if ( T_SocketNonBlocking( *socket ) == SOCKET_ERROR ) {
		TFile_CleanupFailedSocket( "CreateFileServer: Unable to set sock to non-blocking.\n", *socket, result );
		return tfalse;
	}

	// Set socket to reuse address.
	if ( T_SocketReuseAddress( *socket ) == SOCKET_ERROR ) {
		TFile_CleanupFailedSocket( "CreateFileServer: Unable to set socket to reuse address.\n", *socket, result );
		return tfalse;
	}

	// Free up what was allocated from getaddrinfo.
	freeaddrinfo( result );
	return ttrue;
}


/*
====================
TFile_ShutdownFileServer
====================
*/
void TFile_ShutdownFileServer( void ) {
	file_server_initialized = tfalse;
	file_server_running = tfalse;
	if ( closesocket( file_server_socket ) == SOCKET_ERROR ) {
		T_Error( "TFile_ShutdownFileServer: Unable to close file server socket.\n" );
		return;
	}
	T_Print( "File server shutdown.\n" );
}


/*
====================
TFile_InitFileServer
====================
*/
tboolean TFile_InitFileServer( const char *const port ) {
	if ( file_server_initialized ) {
		T_FatalError( "TFile_InitFileServer: File server is already initialized" );
	}
	if ( !CreateFileServer( AF_INET, port, &file_server_socket ) || !CreateFileServer( AF_INET6, port, &file_server_socket6 ) ) {
		T_Error( "TFile_InitFileServer: Unable to initialize file server.\n" );
		return tfalse;
	}
	file_server_initialized = ttrue;
	T_Print( "File server initialized.\n" );
	return ttrue;
}


/*
====================
FileServerThread
====================
*/
static int FileServerThread( void *arg ) {
#define MAX_CONNECTIONS 256
#define MAX_SOCKETS MAX_CONNECTIONS + 2
#define MAX_BUFFER_SIZE 1024

	const SOCKET server = file_server_socket; // fix me
	const SOCKET server6 = file_server_socket6; // fix me

	SOCKET connections[MAX_CONNECTIONS] = { 0 };
	int connectionCount = 0, addrLen = 0;
	struct sockaddr_storage addr;
	byte buffer[MAX_BUFFER_SIZE];

	if ( listen( server, 8 ) == SOCKET_ERROR || listen( server6, 8 ) == SOCKET_ERROR ) {
		T_Error( "FileServerThread: Failed to listen on file server socket." );
		return 0;
	}

	addrLen = sizeof( addr );
	file_server_running = ttrue; // fix me
	while( 1 ) {
		SOCKET sockets[MAX_SOCKETS] = { 0 };
		SOCKET reads[MAX_SOCKETS] = { 0 };
		int i;

		sockets[0] = server;
		sockets[1] = server6;

		if ( !T_Select( sockets, MAX_SOCKETS, 1000000, reads ) ) {
			continue;
		}

		for( i = 0; i < MAX_SOCKETS; ++i ) {
			if ( reads[i] == server ) {
				if ( connections[connectionCount] = accept( server, ( struct sockaddr * )&addr,  &addrLen ) != SOCKET_ERROR ) {
					++connectionCount;
					// TODO
					T_Print( "Client connected.\n" );
				}
			} else if ( reads[i] == server6 ) {
				if ( connections[connectionCount] = accept( server6, ( struct sockaddr * )&addr,  &addrLen ) != SOCKET_ERROR ) {
					++connectionCount;
					// TODO
					T_Print( "Client connected.\n" );
				}
			} else {
				int bytes = recv( reads[i], ( char * )buffer, MAX_BUFFER_SIZE, 0 );
				if ( bytes > 0 ) {
					// TODO
					T_Print( "Server received %i bytes.\n", bytes );
				}
			}
		}
	}
	return 1;
}


/*
====================
TFile_StartFileServer
====================
*/
void TFile_StartFileServer( void ) {
	if ( !file_server_initialized ) {
		T_FatalError( "TFile_StartFileServer: File server is not initialized" );
	}

	if ( file_server_running ) {
		T_FatalError( "TFile_StartFileServer: File server is already running" );
	}

	if ( thrd_create( &file_server_thread, FileServerThread, NULL ) != thrd_success ) {
		T_FatalError( "TFile_StartFileServer: Unable to create thread" );
	}
}

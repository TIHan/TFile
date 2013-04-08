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

#include "t_common.h"
#include "t_socket.h"

typedef struct fileServer_s {
	struct addrinfo info;
	int socket;
} fileServer_t;

static tboolean file_server_initialized = tfalse;
static tboolean file_server_running = tfalse;
static fileServer_t file_server;


/*
====================
CreateFileServer

Passing in the port as a string might be ok, instead of trying to convert
an integer to a string.
====================
*/
static fileServer_t CreateFileServer( const int family, const char *port ) {
	struct addrinfo hints, *result;
	fileServer_t fileServer;

	// We can only create file servers for ipv4 and ipv6.
	if ( family != AF_INET && family != AF_INET6 ) {
		T_Error( "CreateFileServer: Bad socket family type." );
	}

	memset( &fileServer.info, 0, sizeof( fileServer.info ) );
	memset( &hints, 0, sizeof( hints ) );

	fileServer.socket = INVALID_SOCKET;
	hints.ai_family = family;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	if ( getaddrinfo( 0, port, &hints, &result ) == SOCKET_ERROR ) {
		T_Error( "CreateFileServer: Unable to get address information." );
	}

	for ( ; result != 0; result = result->ai_next ) {
		if ( result->ai_family == family ) {
			fileServer.socket = T_socket( result->ai_family, result->ai_socktype, result->ai_protocol );
			break;
		}
	}

	if ( fileServer.socket == INVALID_SOCKET ) {
		freeaddrinfo( result );
		T_Error( "CreateFileServer: Unable to create socket." );
	}

	fileServer.info = *result;

	if ( T_SocketNonBlocking( fileServer.socket ) == SOCKET_ERROR ) {
		T_Error( "CreateFileServer: Unable to make socket non-blocking." );
	}
	if ( T_SocketReuseAddress( fileServer.socket ) == SOCKET_ERROR ) {
		T_Error( "CreateFileServer: Unable to set socket to reuse address." );
	}

	if ( bind( fileServer.socket, fileServer.info.ai_addr, fileServer.info.ai_addrlen ) == SOCKET_ERROR ) {
		T_Error( "CreateFileServer: Unable to bind socket." );
	}

	freeaddrinfo( result );
	return fileServer;
}


/*
====================
TFile_InitFileServer
====================
*/
void TFile_InitFileServer( const char *port ) {
	if ( file_server_initialized ) {
		T_Error( "TFile_InitFileServer: File server is already initialized." );
		return;
	}
	file_server = CreateFileServer( AF_INET, "29760" );
	file_server_initialized = ttrue;
	T_Print( "File server initialized.\n" );
}


/*
====================
TFile_ShutdownFileServer
====================
*/
void TFile_ShutdownFileServer( void ) {
	if ( closesocket( file_server.socket ) == SOCKET_ERROR ) {
		T_Error( "TFile_ShutdownFileServer: Unable to close file server socket." );
		return;
	}
	file_server_initialized = tfalse;
	file_server_running = tfalse;
	T_Print( "File server shutdown.\n" );
}

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

#include "tfile_client.h"

#include "tfile_shared.h"
#include "tinycthread.h"

static SOCKET file_client_socket;


/*
====================
CreateHints
====================
*/
static struct addrinfo CreateFileServerHints( const int family, const int socketType, const int flags ) {
	struct addrinfo hints = T_CreateAddressInfo();

	hints.ai_family = family;
	hints.ai_socktype = socketType;
	hints.ai_flags = flags;
	return hints;
}


/*
====================
CreateFileServer
====================
*/
static tboolean CreateFileClient( const char *const ip, const char *const port, SOCKET *const client ) {
	const struct addrinfo hints = CreateFileServerHints( AF_UNSPEC, SOCK_STREAM, 0 );

	struct addrinfo defaultInfo = T_CreateAddressInfo();
	struct addrinfo *result = &defaultInfo;

	// Get address info.
	if ( getaddrinfo( ip, port, &hints, &result ) == SOCKET_ERROR ) {
		TFile_CleanupFailedSocket( "CreateFileServer: Unable to get address information.\n", INVALID_SOCKET, result );
		return tfalse;
	}

	// Attempt to create a socket.
	*client = T_CreateSocket( AF_UNSPEC, result );
	if ( *client == INVALID_SOCKET ) {
		TFile_CleanupFailedSocket( "CreateFileServer: Unable to create socket.\n", INVALID_SOCKET, result );
		return tfalse;
	}

	// Connect to remote host.
	if ( connect( *client, result->ai_addr, result->ai_addrlen ) == SOCKET_ERROR ) {
		TFile_CleanupFailedSocket( "ConnectFileServer: Unable to connect to remote host.\n", *client, result );
		return tfalse;
	}

	// Set socket to non-blocking.
	if ( T_SocketNonBlocking( *client ) == SOCKET_ERROR ) {
		TFile_CleanupFailedSocket( "CreateFileServer: Unable to set sock to non-blocking.\n", *client, result );
		return tfalse;
	}

	// Free up what was allocated from getaddrinfo.
	freeaddrinfo( result );
	return ttrue;
}


/*
====================
TFile_ConnectFileServer
====================
*/
tboolean TFile_ConnectFileServer( const char *ip, const char *port ) {
	if ( CreateFileClient( ip, port, &file_client_socket ) ) {
		T_Print( "Successfully connected.\n" );
		return ttrue;
	}
	return tfalse;
}
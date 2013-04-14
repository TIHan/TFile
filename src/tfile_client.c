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

#include "t_common.h"
#include "tfile_shared.h"
#include "tinycthread.h"

static SOCKET client_socket = INVALID_SOCKET;
static _bool client_connected = _false;


/*
====================
CreateClient

TODO: Break out normal socket errors.
====================
*/
static _bool CreateClient( const char *const ip, const int port, SOCKET *const socket ) {
	const struct addrinfo hints = T_CreateHints( AF_UNSPEC, SOCK_STREAM, 0 );

	struct addrinfo defaultInfo = T_CreateAddressInfo();
	struct addrinfo *result = &defaultInfo;
	char portStr[MAX_PORT_SIZE];

	T_itoa( port, portStr, MAX_PORT_SIZE );

	// Get address info.
	if ( getaddrinfo( ip, portStr, &hints, &result ) == SOCKET_ERROR ) {
		TFile_CleanupFailedSocket( "CreateClient: Unable to get address information.\n", INVALID_SOCKET, result );
		return _false;
	}

	// Attempt to create a socket.
	*socket = T_CreateSocket( AF_UNSPEC, result );
	if ( *socket == INVALID_SOCKET ) {
		TFile_CleanupFailedSocket( "CreateClient: Unable to create socket.\n", INVALID_SOCKET, result );
		return _false;
	}

	// Connect to remote host.
	if ( connect( *socket, result->ai_addr, result->ai_addrlen ) == SOCKET_ERROR ) {
		TFile_CleanupFailedSocket( "CreateClient: Unable to connect to remote host.\n", *socket, result );
		*socket = INVALID_SOCKET;
		return _false;
	}

	// Set socket to non-blocking.
	if ( T_SocketNonBlocking( *socket ) == SOCKET_ERROR ) {
		TFile_CleanupFailedSocket( "CreateClient: Unable to set socket to non-blocking.\n", *socket, result );
		*socket = INVALID_SOCKET;
		return _false;
	}

	// Free up what was allocated from getaddrinfo.
	freeaddrinfo( result );
	return _true;
}


/*
====================
TFile_ClientDownloadFile
====================
*/
int TFile_ClientDownloadFile( const char *fileName ) {
	_byte cmd = CMD_DOWNLOAD_FILE;
	if ( !client_connected ) {
		T_FatalError( "TFile_DownloadFile: Not connected" );
	}

	if ( send( client_socket, ( char *)&cmd, 1, 0 ) == SOCKET_ERROR ) {
		T_Error( "TFile_DownloadFile: Failed to download file.\n" );
		return _false;
	}
	return _true;
}


/*
====================
TFile_ClientConnect
====================
*/
int TFile_ClientConnect( const char *ip, const int port ) {
	if ( !CreateClient( ip, port, &client_socket ) ) {
		T_Error( "TFile_Connect: Unable to connect to %s.\n", ip );
		return _false;
	}
	client_connected = _true;
	T_Print( "Successfully connected.\n" );
	return _true;
}


/*
====================
TFile_ShutdownClient
====================
*/
void TFile_ShutdownClient( void ) {
	client_connected = _false;
	TFile_TryCloseSocket( client_socket );
	T_Print( "Disconnect from file server.\n" );
}
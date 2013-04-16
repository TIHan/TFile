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

typedef struct {
	SOCKET ip_socket;
} client_message_t;

static cnd_t client_condition;


/*
============================================================================

CLIENT THREAD

============================================================================
*/

#define HEARTBEAT_INTERVAL 1000 // 1 second.

// Socket
static SOCKET server;

// Client Time
static _bool time_initialized;
static _time_t base_time;
static _time_t client_time;

// Heartbeat Time
static _time_t heartbeat_time;


/*
====================
ClientTime
====================
*/
static void ClientTime( void ) {
	client_time = T_Milliseconds( &base_time, ( int * )&time_initialized );
}


/*
====================
HeartbeatTime
====================
*/
static void HeartbeatTime( void ) {
	heartbeat_time = heartbeat_time == 0 ? client_time + HEARTBEAT_INTERVAL : heartbeat_time;
}


/*
====================
TryHeartbeat
====================
*/
static void TryHeartbeat( void ) {
	if ( client_time >= heartbeat_time ) {
		const command_t command = CMD_HEARTBEAT;
		if ( send( server, ( char * )&command, 1, 0 ) <= 0 ) {
			T_Error( "Unable to send heartbeat.\n" );
		}
		heartbeat_time = 0;
	}
}


/*
====================
TryReceive
====================
*/
static void TryReceive( void ) {
	SOCKET read_socket = ZERO_SOCKET;

	if ( T_Select( &server, 1, SELECT_TIMEOUT, &read_socket ) == SOCKET_ERROR ) {
		T_Error( "TryReceive: Select error.\n" );
	}

	// TODO
}


/*
====================
ClientInit
====================
*/
static void ClientInit( const SOCKET socket ) {
	heartbeat_time = 0;
	time_initialized = _false;
	server = socket;
}


/*
====================
HandleMessage
====================
*/
static void HandleMessage( const void *const arg ) {
	const client_message_t message = *( client_message_t * )arg;

	// Initialize client.
	ClientInit( message.ip_socket );

	cnd_signal( &client_condition );
}


/*
====================
ClientThread
====================
*/
static int ClientThread( void *arg ) {
	// Handle the message sent by the calling thread.
	HandleMessage( arg );

	while ( 1 ) {
		// Client's life time.
		ClientTime();

		// Time interval of when to send a heartbeat.
		HeartbeatTime();

		// Try to receive data from the server.
		TryReceive();

		// Try to send a heartbeat.
		TryHeartbeat();
	}
}


/*
============================================================================

CLIENT

============================================================================
*/


static SOCKET client_socket = INVALID_SOCKET;
static _bool client_connected = _false;
static thrd_t client_thread;
static mtx_t client_mutex;


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
	client_message_t message;

	if ( !CreateClient( ip, port, &client_socket ) ) {
		T_Error( "TFile_Connect: Unable to connect to %s.\n", ip );
		return _false;
	}

	cnd_init( &client_condition );
	mtx_init( &client_mutex, mtx_plain );

	message.ip_socket = client_socket;
	if ( thrd_create( &client_thread, ClientThread, &message ) != thrd_success ) {
		T_FatalError( "TFile_ClientConnect: Unable to create thread" );
	}

	cnd_wait( &client_condition, &client_mutex );
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
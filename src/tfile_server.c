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

#include <stdio.h>

typedef struct {
	SOCKET ip_socket;
	SOCKET ip6_socket;
} server_message_t;

static cnd_t server_condition;


/*
============================================================================

SERVER THREAD

============================================================================
*/


#define MAX_CONNECTIONS 254
#define MAX_SOCKETS MAX_CONNECTIONS + 2 // 2 represents IPv4 and IPv6 sockets.
#define CONNECTION_TIMEOUT 5000 // 5 seconds.
#define CHECK_CONNECTIONS_INTERVAL 1000 // 1 second.

// Sockets
static SOCKET server;
static SOCKET server6;
static _byte buffer[MAX_PACKET_SIZE];

// Connections
static SOCKET connections[MAX_CONNECTIONS];
static _time_t connection_times[MAX_CONNECTIONS];
static int connection_count;

// Server Time
static _bool time_initialized;
static _time_t base_time;
static _time_t server_time;

// Check Connections
static _time_t check_connections_time;


/*
====================
AcceptConnection
====================
*/
static void AcceptConnection( const SOCKET socket ) {
	static struct sockaddr_storage addr;
	int len = sizeof( addr );

	if ( ( connections[connection_count] = accept( server, ( struct sockaddr * )&addr,  &len ) ) != SOCKET_ERROR ) {
		connection_times[connection_count] = server_time + CONNECTION_TIMEOUT;
		++connection_count;
		T_Print( "Client connected.\n" );
	}
}


/*
====================
RemoveConnection
====================
*/
static void RemoveConnection( const int connectionIndex ) {
	const int last = MAX_CONNECTIONS - 1;

	int i;

	TFile_TryCloseSocket( connections[connectionIndex] );
	for ( i = connectionIndex; i < last; ++i ) {
		connections[i] = connections[i + 1];
		connection_times[i] = connection_times[i + 1];
	}
	connections[last] = ZERO_SOCKET;
	connection_times[last] = 0;

	--connection_count;
	T_Print( "Client disconnected.\n" );
}


/*
====================
HandlePacket

TODO: This needs a lot more work.
====================
*/
static void HandlePacket( const int connectionIndex ) {
	switch( buffer[0] ) {
	case CMD_HEARTBEAT:
		connection_times[connectionIndex] = server_time + CONNECTION_TIMEOUT;
		break;
	default: // Bad Message
		break;
	}
}


/*
====================
ServerTime
====================
*/
static void ServerTime( void ) {
	server_time = T_Milliseconds( &base_time, ( int * )&time_initialized );
}


/*
====================
ServerTime
====================
*/
static void CheckConnectionsTime( void ) {
	check_connections_time = check_connections_time == 0 ? server_time + CHECK_CONNECTIONS_INTERVAL : check_connections_time;
}


/*
====================
TryCheckConnectionTimes
====================
*/
static void TryCheckConnectionTimes( void ) {
	int i;

	if ( server_time >= check_connections_time ) {
		for( i = 0; i < connection_count; ++i ) {
			if ( server_time >= connection_times[i] ) {
				RemoveConnection( i );
				--i;
			}
		}
		check_connections_time = 0;
	}
}


/*
====================
TryReceive
====================
*/
static void TryReceive( const int timeout ) {
	SOCKET sockets[MAX_SOCKETS] = { ZERO_SOCKET };
	SOCKET reads[MAX_SOCKETS] = { ZERO_SOCKET };
	int i;

	// Set up IPv4 and IPv6 sockets.
	sockets[0] = server;
	sockets[1] = server6;

	for ( i = 2; i < MAX_SOCKETS; ++i ) {
		sockets[i] = connections[i - 2];
	}

	// Synchronous event demultiplexer.
	// It's ok that we are using select as its portable.
	// It may not be the fastest, but it's definitely quick enough for what we are trying to accomplish.
	if ( T_Select( sockets, MAX_SOCKETS, timeout, reads ) == SOCKET_ERROR ) {
		T_Error( "TryReceive: Select error.\n" );
	}

	for( i = 0; i < MAX_SOCKETS; ++i ) {
		if ( reads[i] == ZERO_SOCKET )
			continue;

		// Accept connections on IPv4 and IPv6.
		if ( reads[i] == server ) {
			AcceptConnection( server );
		} else if ( reads[i] == server6 ) {
			AcceptConnection( server6 );
		} else {
			// Handle messages from the accepted connections.
			int bytes = recv( reads[i], ( char * )buffer, MAX_PACKET_SIZE, 0 );
			if ( bytes > 0 ) {
				HandlePacket( i - 2 );
			}
		}
	}
}


/*
====================
ServerInit
====================
*/
static void ServerInit( const SOCKET socket, const SOCKET socket6 ) {
	int i;

	for ( i = 0; i < MAX_CONNECTIONS; ++i ) {
		connections[i] = ZERO_SOCKET;
		connection_times[i] = 0;
	}

	time_initialized = _false;
	check_connections_time = 0;

	server = socket;
	server6 = socket6;

	if ( listen( server, 8 ) == SOCKET_ERROR || listen( server6, 8 ) == SOCKET_ERROR ) {
		T_FatalError( "ServerInit: Failed to listen on socket." );
	}
}


/*
====================
HandleMessage
====================
*/
static void HandleMessage( const void *const arg ) {
	const server_message_t message = *( server_message_t * )arg;

	ServerInit( message.ip_socket, message.ip6_socket );

	cnd_signal( &server_condition );
}

typedef struct {
	FILE *file;
	size_t size;
} tfile_t;


/*
====================
ServerOpenFile
====================
*/
int ServerOpenFile( const char *const fileName, tfile_t *const file ) {
	FILE *const f = fopen( fileName, "rb" );
	size_t size;

	if ( !f ) {
		return _false;
	}

	fseek( f, 0L, SEEK_END );
	size = ftell( f );
	rewind( f );

	file->file = f;
	file->size = size;
	return _true;
}


/*
====================
ServerCloseFile
====================
*/
void *ServerCloseFile( tfile_t *const file ) {
	fclose( file->file );
}


/*
====================
ServerThread
====================
*/
static int ServerThread( void *arg ) {
	// Handle the message sent by the calling thread.
	HandleMessage( arg );

	while( 1 ) {
		// Server's life time.
		ServerTime();

		// Time interval to check connections.
		CheckConnectionsTime();

		// Try to receive data from clients.
		TryReceive( RECEIVE_TIMEOUT );

		// Check to see if any of our accepted connections were dropped.
		TryCheckConnectionTimes();
	}
	return _true;
}


/*
============================================================================

SERVER

============================================================================
*/


static _bool server_initialized = _false;
static _bool server_running = _false;
static SOCKET server_socket = INVALID_SOCKET;
static SOCKET server_socket6 = INVALID_SOCKET;
static thrd_t server_thread;
static mtx_t server_mutex;


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
	const struct addrinfo *found;
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

	// Set socket to reuse address.
	if ( T_SocketReuseAddress( *socket ) == SOCKET_ERROR ) {
		TFile_CleanupFailedSocket( "CreateServer: Unable to set socket to reuse address.\n", *socket, result );
		*socket = INVALID_SOCKET;
		return _false;
	}

	// Bind socket.
	found = T_FindAddrInfo( family, result );
	if ( bind( *socket, found->ai_addr, found->ai_addrlen ) == SOCKET_ERROR ) {
		TFile_CleanupFailedSocket( "CreateServer: Unable to bind socket.\n", *socket, result );
		*socket = INVALID_SOCKET;
		return _false;
	}

	// Set socket to non-blocking.
	if ( T_SocketNonBlocking( *socket ) == SOCKET_ERROR ) {
		TFile_CleanupFailedSocket( "CreateServer: Unable to set socket to non-blocking.\n", *socket, result );
		*socket = INVALID_SOCKET;
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
	TFile_TryCloseSocket( server_socket );
	TFile_TryCloseSocket( server_socket6 );
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
TFile_StartServer
====================
*/
void TFile_StartServer( void ) {
	server_message_t message;

	if ( !server_initialized ) {
		T_FatalError( "TFile_StartServer: Server is not initialized" );
	}

	if ( server_running ) {
		T_FatalError( "TFile_StartServer: Server is already running" );
	}

	cnd_init( &server_condition );
	mtx_init( &server_mutex, mtx_plain );

	message.ip_socket = server_socket;
	message.ip6_socket = server_socket6;
	if ( thrd_create( &server_thread, ServerThread, &message ) != thrd_success ) {
		T_FatalError( "TFile_StartServer: Unable to create thread" );
	}

	cnd_wait( &server_condition, &server_mutex );
	server_running = _true;
}

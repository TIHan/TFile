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

#include "t_socket.h"


/*
====================
T_FindAddrInfo
====================
*/
const struct addrinfo *T_FindAddrInfo( const t_int family, const struct addrinfo *const info ) {
	if ( !info ) {
		return NULL;
	}

	if ( family == info->ai_family || family == AF_UNSPEC ) {
		return info;
	}
	return T_FindAddrInfo( family, info->ai_next );
}


/*
====================
T_CreateSocket
====================
*/
SOCKET T_CreateSocket( const t_int family, const struct addrinfo *const info ) {
	const struct addrinfo *found = T_FindAddrInfo( family, info );
	int onlyV6 = 1;
	SOCKET sockfd;

	if ( !found ) {
		return INVALID_SOCKET;
	}
	sockfd = socket( found->ai_family, found->ai_socktype, found->ai_protocol );

	if ( family == AF_INET6 ) {
		if ( setsockopt( sockfd, IPPROTO_IPV6, IPV6_V6ONLY, ( char * )&onlyV6, sizeof( onlyV6 ) ) == SOCKET_ERROR ) {
			if ( sockfd != INVALID_SOCKET ) {
				closesocket( sockfd );
			}
			return INVALID_SOCKET;
		}

	}

	return sockfd;
}


/*
====================
T_CreateAddressInfo
====================
*/
struct addrinfo T_CreateAddressInfo( void ) {
	struct addrinfo info;

	memset( &info, 0, sizeof( info ) );
	return info;
}


/*
====================
T_SocketReuseAddress
====================
*/
int T_SocketReuseAddress( const SOCKET socket ) {
	int optval = 1;
	return setsockopt( socket, SOL_SOCKET, SO_REUSEADDR, ( char * )&optval, sizeof( optval ) );
}


/*
====================
T_SocketNonBlocking
====================
*/
int T_SocketNonBlocking( const SOCKET socket ) {
#if _WIN32
	u_long argp = 1;
	return ioctlsocket( socket, FIONBIO, &argp );
#else
	return fcntl( socket, F_SETFL, O_NONBLOCK );
#endif
}


/*
====================
T_Select
====================
*/
int T_Select( const SOCKET *const sockets, const t_int size, const t_int usec, SOCKET *const reads ) {
	SOCKET max = 0;
	struct timeval tv;
	fd_set readSet;
	int result;
	int i;

	tv.tv_sec = 0;
	tv.tv_usec = usec;

	FD_ZERO( &readSet );

	for( i = 0; i < size; ++i ) {
		if ( sockets[i] == ZERO_SOCKET )
			continue;

		FD_SET( sockets[i], &readSet );
		if ( sockets[i] > max ) {
			max = sockets[i];
		}
	}

	if ( ( result = select( max + 1, &readSet, 0, 0, &tv ) ) <= 0 ) {
		return result;
	}

	for( i = 0; i < size; ++i ) {
		if ( sockets[i] == ZERO_SOCKET ) {
			reads[i] = ZERO_SOCKET;
			continue;
		}

		if ( FD_ISSET( sockets[i], &readSet ) ) {
			reads[i] = sockets[i];
		} else {
			reads[i] = ZERO_SOCKET;
		}
	}

	return result;
}


/*
====================
T_CreateHints
====================
*/
struct addrinfo T_CreateHints( const t_int family, const t_int socketType, const t_int flags ) {
	struct addrinfo hints = T_CreateAddressInfo();

	hints.ai_family = family;
	hints.ai_socktype = socketType;
	hints.ai_flags = flags;
	return hints;
}

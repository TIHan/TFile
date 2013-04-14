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

#include "t_common.h"

#if _WIN32
#	include <winsock2.h>
#	include <ws2tcpip.h>

#	pragma comment(lib, "ws2_32.lib")
#else
#	include <sys/socket.h>
#	include <sys/unistd.h>
#	include <sys/fcntl.h>
#	include <netdb.h>
#	include <arpa/inet.h>
#	include <netinet/in.h>
#	include <cstring>
#	include <stdlib.h>
#	include <unistd.h>

typedef int SOCKET;
#	define INVALID_SOCKET		-1
#	define SOCKET_ERROR			-1
#	define closesocket			close
#endif

SOCKET T_CreateSocket( const int family, const struct addrinfo *const info );
struct addrinfo T_CreateAddressInfo( void );
int T_SocketReuseAddress( const SOCKET socket );
int T_SocketNonBlocking( const SOCKET socket );
int T_Select( const SOCKET *const sockets, const int size, const int usec, SOCKET *const reads );
struct addrinfo T_CreateHints( const int family, const int socketType, const int flags );

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

#include "tfile_shared.h"


/*
====================
TFile_CleanupFailedSocket
====================
*/
void TFile_CleanupFailedSocket( const t_char *const error, const SOCKET socket, struct addrinfo *const info ) {
	if ( info ) {
		freeaddrinfo( info );
	}
	if ( error ) {
		T_Error( error );
	}
	TFile_TryCloseSocket( socket );
}


/*
====================
TFile_TryCloseSocket
====================
*/
t_bool TFile_TryCloseSocket( const SOCKET socket ) {
	if ( socket == INVALID_SOCKET )
		return t_false;

	if ( closesocket( socket ) == SOCKET_ERROR ) {
		T_Error( "TFile_TryCloseSocket: Error trying to close valid socket.\n" );
		return t_false;
	}
	return t_true;
}

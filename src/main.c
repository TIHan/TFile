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

#include "tfile.h"
#include "t_common.h"
#include <stdio.h>


/*
====================
main
====================
*/
int main( void ) {
	t_byteStream_t *byteStream = T_CreateByteStream( 1024 );
	char read[256];
	int host;

#ifdef _WIN32
	TFile_InitWinsock();
#endif

	T_BSWriteString( byteStream, "Hello world!" );
	T_BSReadString( byteStream, read, 256 );

	printf( "READ BYTE: %s\n", read );
	printf("Can you host? ");
	scanf("%d", &host);
	getchar();

	if ( host == 1 ) {
		if ( !TFile_InitServer( 27960 ) ) {
			getchar();
			return 0;
		}

		TFile_StartServer();
	}

	if ( !TFile_ClientConnect( "127.0.0.1", 27960 ) ) {
		getchar();
		return 0;
	}

	getchar();

	TFile_ShutdownClient();

	getchar();

	if ( host == 1 )
		TFile_ShutdownServer();

#ifdef _WIN32
	TFile_ShutdownWinsock();
#endif
	return 1;
}

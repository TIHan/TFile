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

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#define MAX_CHAR_SIZE 4096



/*
====================
T_FatalError
====================
*/
void T_FatalError( const char *const format, ... ) {
	va_list list;

	va_start( list, format );
	vfprintf( stderr, format, list );
	va_end( list );

	getchar();
	exit( 1 );
}


/*
====================
T_Error
====================
*/
void T_Error( const char *const format, ... ) {
	va_list list;

	va_start( list, format );
	vfprintf( stderr, format, list );
	va_end( list );
}


/*
====================
T_Print
====================
*/
void T_Print( const char *const format, ... ) {
	va_list list;

	va_start( list, format );
	vprintf( format, list );
	va_end( list );
}


/*
====================
T_itoa

// TODO: Fix me
====================
*/
void T_itoa( const int value, char *const destination, const int size ) {
	itoa( value, destination, 10 );
}


/*
====================
T_getchar
====================
*/
int T_getchar( void ) {
	return getchar();
}

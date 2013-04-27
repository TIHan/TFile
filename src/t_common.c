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

#include "t_internal_types.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>


struct tbyteStream_s {
	int size;
	int maxSize;
	int readPosition;
	int writePosition;
	_byte *buffer;
};


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
T_CreateByteStream
====================
*/
tbyteStream_t *T_CreateByteStream( const int size ) {
	tbyteStream_t *byteStream = ( tbyteStream_t * )malloc( sizeof( tbyteStream_t ) );

	byteStream->size = 0;
	byteStream->maxSize = size;
	byteStream->readPosition = 0;
	byteStream->writePosition = 0;
	byteStream->buffer = ( _byte * )malloc( size );
	memset( byteStream->buffer, 0, size );

	return byteStream;
}


/*
====================
T_BSWriteByte
====================
*/
void T_BSWriteByte( tbyteStream_t *const byteStream, const _byte value ) {
	if ( byteStream->size >= byteStream->maxSize ) {
		T_Error( "T_BSWriteByte: Unable to write byte to stream.\n" );
		return;
	}
	byteStream->buffer[byteStream->writePosition++] = value;
}


/*
====================
T_BSReadByte
====================
*/
_byte T_BSReadByte( tbyteStream_t *const byteStream ) {
	if ( byteStream->size >= byteStream->maxSize ) {
		T_Error( "T_BSReadByte: Unable to read byte from stream.\n" );
		return 0;
	}
	return byteStream->buffer[byteStream->readPosition++];
}


/*
====================
_T_itoa_reverse
====================
*/
static void _T_itoa_reverse( const int position, char *const destination, const int length ) {
	const char first = destination[position];
	const char last = destination[length - position];

	if ( first == '-' ) {
		_T_itoa_reverse( position + 1, destination, length + 1 );
		return;
	}

	destination[position] = last;
	destination[length - position] = first;
	if ( position == length / 2 ) {
		return;
	}
	_T_itoa_reverse( position + 1, destination, length );
}


/*
====================
_T_itoa_convert
====================
*/
static void _T_itoa_convert( const int value, const int length, char *const destination, const int size ) {
	if ( length == size - 1 || value == 0 ) {
		_T_itoa_reverse( 0, destination, length - 1 );
		destination[length] = '\0';
		return;
	}
	destination[length] = value % 10 + '0';
	_T_itoa_convert( value / 10, length + 1, destination, size );
}


/*
====================
T_itoa
====================
*/
void T_itoa( const int value, char *const destination, const int size ) {
	if ( size <= 1 ) {
		return;
	}

	if ( value < 0 ) {
		destination[0] = '-';
		_T_itoa_convert( value * -1, 1, destination, size );
		return;
	} else if ( value == 0 ) {
		destination[0] = '0';
		destination[1] = '\0';
		return;
	}
	_T_itoa_convert( value, 0, destination, size );
}

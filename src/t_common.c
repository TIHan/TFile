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

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>


struct t_byteStream_s {
	int size;
	int maxSize;
	int readPosition;
	int writePosition;
	t_byte *buffer;
};


/*
====================
T_FatalError
====================
*/
void T_FatalError( const t_char *const format, ... ) {
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
void T_Error( const t_char *const format, ... ) {
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
void T_Print( const t_char *const format, ... ) {
	va_list list;

	va_start( list, format );
	vprintf( format, list );
	va_end( list );
}


/*
====================
T_Malloc
====================
*/
void *T_Malloc( const t_uint size ) {
	return malloc( size );
}


/*
====================
T_Malloc0
====================
*/
void *T_Malloc0( const t_uint size ) {
	void *const memory = malloc( size );
	memset( memory, 0, size );
	return memory;
}


/*
====================
T_Free
====================
*/
void T_Free( void *const memory ) {
	free( memory );
}


/*
====================
T_CreateByteStream
====================
*/
t_byteStream_t *T_CreateByteStream( const t_int size ) {
	t_byteStream_t *const byteStream = ( t_byteStream_t * )T_Malloc( sizeof( t_byteStream_t ) );

	byteStream->size = 0;
	byteStream->maxSize = size;
	byteStream->readPosition = 0;
	byteStream->writePosition = 0;
	byteStream->buffer = T_Malloc( size );

	return byteStream;
}


/*
====================
T_DestroyByteStream
====================
*/
void T_DestroyByteStream( t_byteStream_t *const byteStream ) {
	T_Free( byteStream->buffer );
	T_Free( byteStream );
}


/*
====================
T_BSReset
====================
*/
void T_BSReset( t_byteStream_t *const byteStream ) {
	byteStream->size = 0;
	byteStream->readPosition = 0;
	byteStream->writePosition = 0;
}


/*
====================
T_BSCanRead
====================
*/
t_bool T_BSCanRead( const t_byteStream_t *const byteStream ) {
	if ( byteStream->readPosition >= byteStream->size )
		return t_false;

	return t_true;
}


/*
====================
T_BSCanWrite
====================
*/
t_bool T_BSCanWrite( const t_byteStream_t *const byteStream ) {
	if ( byteStream->size >= byteStream->maxSize )
		return t_false;

	return t_true;
}


/*
====================
T_BSWriteByte
====================
*/
void T_BSWriteByte( t_byteStream_t *const byteStream, const t_byte value ) {
	if ( !T_BSCanWrite( byteStream ) ) {
		T_Error( "T_BSWriteByte: Unable to write byte to stream.\n" );
		return;
	}
	byteStream->buffer[byteStream->writePosition++] = value;
	++byteStream->size;
}


/*
====================
T_BSReadByte
====================
*/
t_byte T_BSReadByte( t_byteStream_t *const byteStream ) {
	if ( !T_BSCanRead( byteStream ) ) {
		T_Error( "T_BSReadByte: Unable to read byte from stream.\n" );
		return 0;
	}
	return byteStream->buffer[byteStream->readPosition++];
}


/*
====================
T_BSWriteBuffer
====================
*/
void T_BSWriteBuffer( t_byteStream_t *const byteStream, const t_byte *const buffer, const t_int size ) {
	int i;

	for ( i = 0; i < size; ++i ) {
		T_BSWriteByte( byteStream, buffer[i] );
	}
}


/*
====================
T_BSWriteString
====================
*/
void T_BSWriteString( t_byteStream_t *const byteStream, const t_char *const str ) {
	int i = 0;

	do {
		const t_char ch = str[i];

		T_BSWriteByte( byteStream, ch );
		if ( ch == '\0' ) {
			break;
		}
		++i;
	} while ( 1 );
}


/*
====================
T_BSReadString
====================
*/
void T_BSReadString( t_byteStream_t *const byteStream, t_char *const str, const t_int size ) {
	int i = 0;

	do {
		const t_char ch = T_BSReadByte( byteStream );

		if ( ( i >= size - 1 ) || ( ch == '\0' ) ) {
			str[i] = '\0';
			break;
		}
		str[i++] = ch;
	} while ( 1 );
}


/*
====================
T_BSGetBuffer
====================
*/
t_byte *T_BSGetBuffer( const t_byteStream_t *const byteStream ) {
	return byteStream->buffer;
}


/*
====================
T_BSGetSize
====================
*/
t_int T_BSGetSize( const t_byteStream_t *const byteStream ) {
	return byteStream->size;
}


/*
====================
_T_itoa_reverse
====================
*/
static void _T_itoa_reverse( const t_int position, t_char *const destination, const t_int length ) {
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
static void _T_itoa_convert( const t_int value, const t_int length, t_char *const destination, const t_int size ) {
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
void T_itoa( const t_int value, t_char *const destination, const t_int size ) {
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

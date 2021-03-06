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

#ifndef _T_COMMON_H_
#define _T_COMMON_H_

#define T_FUNC( name, retType, argType ) retType ( *name )( argType * )

typedef unsigned char t_byte;
typedef char t_char;
typedef unsigned short t_ushort;
typedef short t_short;

#ifdef _WIN32
typedef __int32 t_int;
typedef unsigned __int32 t_uint;
typedef __int64 t_int64;
typedef unsigned __int64 t_uint64;
#else
#endif

typedef enum {
	t_false,
	t_true
} t_bool;

typedef struct t_byteStream_s t_byteStream_t;

t_byteStream_t *T_CreateByteStream( const t_int size );
void T_DestroyByteStream( t_byteStream_t *const byteStream );
void T_BSReset( t_byteStream_t *const byteStream );
t_bool T_BSCanRead( const t_byteStream_t *const byteStream );
t_bool T_BSCanWrite( const t_byteStream_t *const byteStream );
void T_BSWriteByte( t_byteStream_t *const byteStream, const t_byte value );
t_byte T_BSReadByte( t_byteStream_t *const byteStream );
t_byte *T_BSGetBuffer( const t_byteStream_t *const byteStream );
t_int T_BSGetSize( const t_byteStream_t *const byteStream );
void T_BSWriteBuffer( t_byteStream_t *const byteStream, const t_byte *const buffer, const t_int size );
void T_BSWriteString( t_byteStream_t *const byteStream, const t_char *const str );
void T_BSReadString( t_byteStream_t *const byteStream, t_char *const str, const t_int size );

#define T_BSWrite( byteStream, type, write ) \
{ \
	const t_int __size = sizeof( type ); \
	\
	int __i; \
	\
	union { \
		t_byte buffer[sizeof( type )]; \
		type writeValue; \
	} __pack; \
	\
	__pack.writeValue = write; \
	\
	for ( __i = 0; __i < __size; ++__i ) { \
		T_BSWriteByte( byteStream, __pack.buffer[__i] ); \
	} \
}

#define T_BSRead( byteStream, type, read ) \
{ \
	const t_int __size = sizeof( type ); \
	\
	int __i; \
	\
	union { \
		t_byte buffer[sizeof( type )]; \
		type readValue; \
	} __unpack; \
	\
	for ( __i = 0; __i < __size; ++__i ) { \
		__unpack.buffer[__i] = T_BSReadByte( byteStream ); \
	} \
	\
	read = __unpack.readValue; \
}

void T_FatalError( const t_char *const format, ... );
void T_Error( const t_char *const format, ... );
void T_Print( const t_char *const format, ... );
void *T_Malloc( const t_uint size );
void *T_Malloc0( const t_uint size );
void T_Free( void *const memory );

t_uint64 T_Milliseconds( t_uint64 *const baseTime, t_int *const initialized );

void T_itoa( const t_int value, t_char *const destination, const t_int size );

#endif // _T_COMMON_H_

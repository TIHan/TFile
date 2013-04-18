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

#include "t_pipe.h"

#include "t_internal_types.h"
#include "tinycthread.h"
#include <stdlib.h>

typedef struct __linked_pipe_data_s __linked_pipe_data_t;
typedef struct __linked_pipe_data_s {
	void *data;
	__linked_pipe_data_t *next;
} __linked_pipe_data_t;

typedef struct tpipe_s {
	void *data;
	mtx_t mutex;
	mtx_t mutexBuffer;
	__linked_pipe_data_t *linked;
	__linked_pipe_data_t *linkedBuffer;
	__linked_pipe_data_t *current;
	__linked_pipe_data_t *currentBuffer;
} tpipe_t;


/*
====================
T_PipeInit
====================
*/
void T_PipeInit( tpipe_t *pipe ) {
	pipe = malloc( sizeof( pipe ) );
	mtx_init( &pipe->mutex, mtx_try );
	mtx_init( &pipe->mutexBuffer, mtx_try );

	pipe->data = NULL;
	pipe->linked = NULL;
	pipe->linkedBuffer = NULL;
	pipe->current = NULL;
	pipe->currentBuffer = NULL;
}


/*
====================
T_PipeSend
====================
*/
int T_PipeSend( tpipe_t *const pipe, void *const message ) {
	if ( mtx_trylock( &pipe->mutex ) != thrd_success ) {
		if ( !pipe->current ) {
			pipe->linkedBuffer = malloc( sizeof( __linked_pipe_data_t ) );
			pipe->currentBuffer = pipe->linkedBuffer;
		} else {
			pipe->currentBuffer->next = malloc( sizeof( __linked_pipe_data_t ) );
			pipe->currentBuffer = pipe->current->next;
		}
		pipe->currentBuffer->data = message;
		pipe->currentBuffer->next = NULL;
		return _false;
	}

	if ( pipe->linked ) {
		if ( pipe->linkedBuffer ) {
			pipe->linked->next = pipe->linkedBuffer;
			pipe->current = pipe->currentBuffer;
			pipe->linkedBuffer = NULL;
			pipe->currentBuffer = NULL;
		}
		pipe->current->next = malloc( sizeof( __linked_pipe_data_t ) );
		pipe->current = pipe->current->next;
	} else {
		if ( pipe->linkedBuffer ) {
			pipe->linked = pipe->linkedBuffer;
			pipe->current = pipe->currentBuffer;
			pipe->linkedBuffer = NULL;
			pipe->currentBuffer = NULL;
		} else {
			pipe->linked = malloc( sizeof( __linked_pipe_data_t ) );
			pipe->current = pipe->linked;
		}
	}

	pipe->current->next->data = message;
	pipe->current->next->next = NULL;

	mtx_unlock( &pipe->mutex );
	return _true;
}


/*
====================
__T_PipeReceive_iterate
====================
*/
void __T_PipeReceive_iterate( __linked_pipe_data_t *linked, void ( *iterate )( void * ) ) {
	if ( !linked ) {
		return;
	}

	iterate( linked->data );
	__T_PipeReceive_iterate( linked->next, iterate );
	free( linked );
}


/*
====================
T_PipeReceive
====================
*/
int T_PipeReceive( tpipe_t *const pipe, void ( *iterate )( void * ) ) {
	if ( mtx_trylock( &pipe->mutex ) != thrd_success ) {
		return _false;
	}
	__T_PipeReceive_iterate( pipe->linked, iterate );
	pipe->linked = NULL;
	mtx_unlock( &pipe->mutex );
	return _true;
}


/*
====================
T_DestroyPipe
====================
*/
void T_DestroyPipe( tpipe_t *const pipe ) {
	mtx_destroy( &pipe->mutex );
	free( pipe );
}

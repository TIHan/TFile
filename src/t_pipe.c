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

#include "tinycthread.h"

#include <stdlib.h>

typedef struct __linked_pipe_data_s __linked_pipe_data_t;
typedef struct __linked_pipe_data_s {
	void *data;
	__linked_pipe_data_t *next;
} __linked_pipe_data_t;

struct t_pipe_s {
	void *data;
	mtx_t mutex;
	mtx_t mutexBuffer;
	__linked_pipe_data_t *linked;
	__linked_pipe_data_t *linkedBuffer;
	__linked_pipe_data_t *last;
	__linked_pipe_data_t *lastBuffer;
};



/*
====================
T_PipeInit
====================
*/
t_pipe_t *T_CreatePipe( void ) {
	t_pipe_t *pipe = ( t_pipe_t * )T_Malloc( sizeof( t_pipe_t ) );

	if ( mtx_init( &pipe->mutex, mtx_try ) != thrd_success ) {
		return NULL;
	}
	if ( mtx_init( &pipe->mutexBuffer, mtx_try ) != thrd_success ) {
		return NULL;
	}

	pipe->data = pipe;
	pipe->linked = NULL;
	pipe->linkedBuffer = NULL;
	pipe->last = NULL;
	pipe->lastBuffer = NULL;
	return pipe;
}


/*
====================
SendBuffer
====================
*/
static void SendBuffer( t_pipe_t *const pipe, void *const message ) {
	if ( pipe->linkedBuffer ) {
		pipe->lastBuffer->next = T_Malloc( sizeof( __linked_pipe_data_t ) );
		pipe->lastBuffer = pipe->lastBuffer->next;
	} else {
		pipe->linkedBuffer = T_Malloc( sizeof( __linked_pipe_data_t ) );
		pipe->lastBuffer = pipe->linkedBuffer;
	}
	pipe->lastBuffer->data = message;
	pipe->lastBuffer->next = NULL;
}


/*
====================
Send
====================
*/
static void Send( t_pipe_t *const pipe, void *const message ) {
	if ( pipe->linked ) {
		pipe->last->next = T_Malloc( sizeof( __linked_pipe_data_t ) );
		pipe->last = pipe->last->next;
	} else {
		pipe->linked = T_Malloc( sizeof( __linked_pipe_data_t ) );
		pipe->last = pipe->linked;
	}
	pipe->last->data = message;
	pipe->last->next = NULL;
}


/*
====================
T_PipeSend
====================
*/
t_bool T_PipeSend( t_pipe_t *const pipe, void *const message ) {
	if ( mtx_trylock( &pipe->mutex ) != thrd_success ) {
		mtx_lock( &pipe->mutexBuffer );
		SendBuffer( pipe, message );
		mtx_unlock( &pipe->mutexBuffer );
		return t_false;
	}

	Send( pipe, message );
	mtx_unlock( &pipe->mutex );
	return t_true;
}


/*
====================
ReceiveBuffer
====================
*/
static void ReceiveBuffer( t_pipe_t *const pipe ) {
	if ( pipe->linkedBuffer ) {
		pipe->lastBuffer->next = pipe->linked;
		pipe->linked = pipe->linkedBuffer;
		pipe->linkedBuffer = NULL;
		pipe->lastBuffer = NULL;
	}
}


/*
====================
__T_PipeReceive_iterate
====================
*/
static void __T_PipeReceive_iterate( __linked_pipe_data_t *linked, void ( *iterate )( void * ) ) {
	if ( !linked ) {
		return;
	}

	iterate( linked->data );
	__T_PipeReceive_iterate( linked->next, iterate );
	T_Free( linked );
}


/*
====================
T_PipeReceive
====================
*/
t_bool T_PipeReceive( t_pipe_t *const pipe, void ( *iterate )( void * ) ) {
	if ( mtx_trylock( &pipe->mutex ) != thrd_success ) {
		return t_false;
	}

	mtx_lock( &pipe->mutexBuffer );
	ReceiveBuffer( pipe );
	mtx_unlock( &pipe->mutexBuffer );

	__T_PipeReceive_iterate( pipe->linked, iterate );
	pipe->linked = NULL;

	mtx_unlock( &pipe->mutex );
	return t_true;
}


/*
====================
T_DestroyPipe

TODO: free linked list
====================
*/
void T_DestroyPipe( t_pipe_t *const pipe ) {
	mtx_destroy( &pipe->mutex );
	mtx_destroy( &pipe->mutexBuffer );
	T_Free( pipe );
}

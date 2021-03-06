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

#include <Windows.h>
#pragma comment(lib, "Winmm.lib")

/*
====================
T_Milliseconds
====================
*/
t_uint64 T_Milliseconds( t_uint64 *const baseTime, t_int *const initialized ) {
// Which should we use? Could we use both in a mixture?
#if 0
#define CAST_MILLISECONDS( time, frequency ) \
	( time.QuadPart * ( 1000.0 / frequency.QuadPart ) ) \

    LARGE_INTEGER frequency;
    LARGE_INTEGER time;

    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&time);

	if ( !( *initialized ) ) {
		*baseTime = ( _time_t )CAST_MILLISECONDS( time, frequency );
		*initialized = t_true;
	}

	return ( _time_t )CAST_MILLISECONDS( time, frequency ) - *baseTime;
#else
	if ( !( *initialized ) ) {
		*baseTime = timeGetTime();
		*initialized = t_true;
	}

	return timeGetTime() - *baseTime;
#endif
}

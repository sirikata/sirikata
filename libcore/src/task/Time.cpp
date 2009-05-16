/*  Sirikata Kernel -- Task scheduling system
 *  Time.cpp
 *
 *  Copyright (c) 2008, Patrick Reiter Horn
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Sirikata nor the names of its contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "util/Standard.hh"
#include "Time.hpp"

#ifndef _WIN32
#include <sys/time.h>
#endif
#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>
// probably something like this function

Sirikata::Task::AbsTime Sirikata::Task::AbsTime::now() {
	FILETIME ft;
	GetSystemTimeAsFileTime(&ft);
	ULARGE_INTEGER uli;
	uli.LowPart = ft.dwLowDateTime;
	uli.HighPart = ft.dwHighDateTime;
	ULONGLONG time64 = uli.QuadPart/10;
	return AbsTime::microseconds(time64);
}

#else
Sirikata::Task::AbsTime Sirikata::Task::AbsTime::now() {
	struct timeval tv = {0, 0};
	gettimeofday(&tv, NULL);
    uint64 total_time=tv.tv_sec;
    total_time*=1000000;
    total_time+=tv.tv_usec;
	return AbsTime::microseconds(total_time);
}
#endif

namespace Sirikata { namespace Task {

std::ostream& operator<<(std::ostream& os, const DeltaTime& rhs) {
    os << rhs.toMilliseconds() << "ms";
    return os;
}

std::istream& operator>>(std::istream& is, DeltaTime& rhs) {
    double v;
    is >> v;

    char type;
    is >> type;
    if (type == 's')
        rhs = DeltaTime::seconds((float)v);
    else if (type == 'm') {
        is >> type;
        assert(type == 's');
        rhs = DeltaTime::milliseconds((float)v);
    }
    else if (type == 'u') {
        is >> type;
        assert(type == 's');
        rhs = DeltaTime::microseconds((uint64)v);
    }

    return is;
}
} }

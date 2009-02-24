/*  cbr
 *  Duration.hpp
 *
 *  Copyright (c) 2009, Ewen Cheslack-Postava
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
 *  * Neither the name of cbr nor the names of its contributors may
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

#ifndef _CBR_DURATION_HPP_
#define _CBR_DURATION_HPP_

#include "Utility.hpp"

namespace CBR {

class Time;

class Duration {
public:
    Duration(uint64 microsecs);
    Duration(const Duration& cpy);
    ~Duration();

    static Duration seconds(float32 dt);
    static Duration seconds(uint32 dt);
    static Duration milliseconds(float32 dt);
    static Duration milliseconds(uint32 dt);

    float32 seconds() const;
    float32 milliseconds() const;

    Duration operator+(const Duration& rhs) const;
    Duration& operator+=(const Duration& rhs);

    Time operator+(const Time& rhs) const;

    Duration operator-(const Duration& rhs) const;
    Duration& operator-=(const Duration& rhs);

    bool operator<(const Duration& rhs) const;
    bool operator==(const Duration& rhs) const;
private:
    friend class Time;

    Duration();

    int64 mMicrosecs;
}; // class Duration

} // namespace CBR

#endif //_CBR_DURATION_HPP_

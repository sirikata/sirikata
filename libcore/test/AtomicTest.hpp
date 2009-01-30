/*  Sirikata Tests -- Sirikata Test Suite
 *  AtomicTest.hpp
 *
 *  Copyright (c) 2008, Daniel Reiter Horn
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
#include <cxxtest/TestSuite.h>
#include "util/AtomicTypes.hpp"

using namespace Sirikata;
class AtomicTest : public CxxTest::TestSuite
{
public:
    void setUp( void )
    {
    }
    void tearDown( void )
    {
    }
    void testAtomicIncrement32( void ) {
        int32_t test=1235918251;
        AtomicValue<int32_t> a;
        a=test;
        a++;
        a+=235;
        int32_t output=a.read();
        TS_ASSERT_EQUALS(test+1+235,output);
    }
    void testAtomicIncrement32u( void ) {
        uint32_t test=3235918251U;
        AtomicValue<uint32_t> a;
        a=test;
        a++;
        a+=235;
        uint32_t output=a.read();
        TS_ASSERT_EQUALS(test+1+235,output);
    }
    void testAtomicIncrement64( void ) {
        int64_t test=65535;
        test*=65535;
        test*=65535;
        AtomicValue<int64_t> a;
        a=test;
        a++;
        a+=235;
        int64_t output=a.read();
        TS_ASSERT_EQUALS(test+1+235,output);
    }
};

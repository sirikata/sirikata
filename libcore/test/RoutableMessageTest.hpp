/*  Sirikata Tests -- Sirikata Test Suite
 *  RoutableMessageTest.hpp
 *
 *  Copyright (c) 2009, Patrick Reiter Horn
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

#include "util/RoutableMessageHeader.hpp"
#include "util/SpaceObjectReference.hpp"

using namespace Sirikata;

class RoutableMessageTest : public CxxTest::TestSuite
{
public:
    void testSerializeHeader() {
        const uint32 TEST_DESTINATION_PORT = 100; //one byte
        const uint32 TEST_SOURCE_PORT = 1000000; // three bytes
        RoutableMessageHeader header, headerOut;
        ObjectReference testID(UUID::random());
        header.set_destination_object(testID);
        header.set_destination_port(TEST_DESTINATION_PORT);
        header.set_source_port(TEST_SOURCE_PORT);
        String headerStr;
        header.SerializeToString(&headerStr);
        headerOut.ParseFromString(headerStr);
        TS_ASSERT(headerOut.has_destination_object());
        TS_ASSERT_EQUALS(testID, headerOut.destination_object());
        TS_ASSERT_EQUALS(TEST_SOURCE_PORT, headerOut.source_port());
        TS_ASSERT_EQUALS(TEST_DESTINATION_PORT, headerOut.destination_port());
    }
};

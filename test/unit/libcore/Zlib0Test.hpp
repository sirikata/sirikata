/*  Sirikata Jpeg Texture Transfer
 *  Zlib0 Test
 *
 *  Copyright (c) 2015, Daniel Reiter Horn
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
#include <memory>
#include <cstring>
#include <sirikata/core/util/Paths.hpp>
#include <sirikata/core/jpeg-arhc/BumpAllocator.hpp>
#include <sirikata/core/jpeg-arhc/Zlib0.hpp>

class Zlib0Test : public CxxTest::TestSuite
{
public:
    void testShortString( void ) {
        using namespace Sirikata;
        JpegAllocator<uint8_t>alloc;
        {
            MemReadWriter base(alloc);
            {
                Zlib0Writer wr(&base, 0);
                wr.setFullFileSize(7);
                wr.Write((uint8_t*)"HELLOTE",7);
                wr.Close();
                uint8_t gold[] = {0x78, 0x01, // magic
                                  0x01, // block type
                                  0x07, 0x00, 0xf8, 0xff, //size
                                  0x48, 0x45, 0x4c, 0x4c, 0x4f, 0x54, 0x45, // payload
                                  0x08, 0x23, 0x02, 0x0e}; // adler32
                TS_ASSERT_EQUALS(base.buffer().size(), sizeof(gold));
                TS_ASSERT(memcmp(gold, &base.buffer()[0], sizeof(gold)) == 0);
            }
        }
    }

};

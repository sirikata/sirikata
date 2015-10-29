/*  Sirikata Jpeg Texture Transfer -- Compression Reader/Writer implementations
 *  Compression Test
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

#include <sirikata/core/jpeg-arhc/ZlibCompression.hpp>
#ifdef __linux
#include <sys/wait.h>
#include <linux/seccomp.h>

#include <sys/prctl.h>
#include <sys/syscall.h>
#endif

class CompressionZlibTest : public CxxTest::TestSuite
{
    size_t loadFileHelper(FILE *input, size_t input_size, Sirikata::MemReadWriter&original,
                    const Sirikata::JpegAllocator<uint8_t> &alloc) {
        using namespace Sirikata;
        std::vector<uint8, JpegAllocator<uint8> > inputData(alloc);
        inputData.resize(input_size);
        if (!inputData.empty()) {
            fread(&inputData[0], inputData.size(), 1, input);
            original.Write(&inputData[0], inputData.size());
        }
        return inputData.size();
    }
    std::pair<FILE *, size_t> getFileObjectAndSize(const char *fileName) {
        using namespace Sirikata;
        String collada_data_dir = Path::Get(Path::DIR_EXE);
        // Windows exes are one level deeper due to Debug or RelWithDebInfo
#if SIRIKATA_PLATFORM == SIRIKATA_PLATFORM_WINDOWS
        collada_data_dir = collada_data_dir +  "/../";
#endif
        collada_data_dir = collada_data_dir + "/../../test/unit/libmesh/collada/";
        String curFile = collada_data_dir + fileName;
        FILE * input = fopen(curFile.c_str(), "rb");
        TS_ASSERT_EQUALS(!input, false);
        size_t input_size = 0;
        if (input) {
            fseek(input, 0, SEEK_END);
            input_size = ftell(input);
            fseek(input, 0, SEEK_SET);
        }
        return std::pair<FILE*, size_t>(input, input_size);
    }

    size_t loadFile(const char *fileName, Sirikata::MemReadWriter&original,
                    const Sirikata::JpegAllocator<uint8_t> &alloc) {
        std::pair<FILE*, size_t> input = getFileObjectAndSize(fileName);
        size_t retval = loadFileHelper(input.first, input.second, original, alloc);
        fclose(input.first);
        return retval;

    }
public:

    void testGenericCompressedRoundTrip( void )
    {
        genericCompressedRoundTripHelper(Sirikata::JpegAllocator<uint8_t>());
    }
    void genericCompressedRoundTripHelper(const Sirikata::JpegAllocator<uint8_t> &alloc)
    {
        using namespace Sirikata;
        MemReadWriter original(alloc);
        size_t inputDataSize = loadFile("prism/texture0.jpg", original, alloc);
        MemReadWriter zz(alloc);
        JpegError err = CompressAnytoZlib(original, zz, 9, alloc);
        if (err != JpegError()) {
            fprintf(stderr, "7Z compression Error: %s\n", err.what());
        }
        TS_ASSERT_EQUALS(err, JpegError());
        MemReadWriter round(alloc);
        err = DecompressZlibtoAny(zz, round, alloc);
        TS_ASSERT_EQUALS(err, JpegError());
        TS_ASSERT_EQUALS(original.buffer(), round.buffer());
        TS_ASSERT_LESS_THAN(zz.buffer().size(), inputDataSize * 99 / 100);
        TS_ASSERT_LESS_THAN(0, zz.buffer().size());
        if (zz.buffer().size()) {
            TS_ASSERT_EQUALS(zz.buffer()[0], 0x78);
        }
        
    }
    void testGenericCompressedRoundTripBumpAllocator( void )
    {
        using namespace Sirikata;
        JpegAllocator<uint8_t> alloc;
        alloc.setup_memory_subsystem(768 * 1024 * 1024,//4294967295,
                                     16,
                                     &BumpAllocatorInit,
                                     &BumpAllocatorMalloc,
                                     &BumpAllocatorFree,
                                     &BumpAllocatorRealloc,
                                     &BumpAllocatorMsize);
        genericCompressedRoundTripHelper(alloc);
        alloc.teardown_memory_subsystem(&BumpAllocatorDestroy);
    }
};

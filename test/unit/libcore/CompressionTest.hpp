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

#include <sirikata/core/jpeg-arhc/SwitchableCompression.hpp>
#include <sirikata/core/jpeg-arhc/MultiCompression.hpp>
#ifdef __linux
#include <sys/wait.h>
#include <linux/seccomp.h>

#include <sys/prctl.h>
#include <sys/syscall.h>
#endif

class CompressionTest : public CxxTest::TestSuite
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
    class SingleFaultInjector : public Sirikata::FaultInjectorXZ {
        int faultingThread;
        int faultingOffset;
    public:
        SingleFaultInjector(int offset, int thread) {
            faultingOffset = offset;
            faultingThread = thread;
        }
        bool shouldFault(int offset, int thread, int nThreads) {
            return offset == faultingOffset && thread == faultingThread;
        }
    };
    void testMultithreadedCompressedRoundTrip( void )
    {
        SingleFaultInjector first(0,0);
        helperMultithreadedCompressedRoundTrip(first, 8, 1.08);
        SingleFaultInjector fi(0,1);
        helperMultithreadedCompressedRoundTrip(fi, 4, 1.01);

        SingleFaultInjector fiA(1,1);
        helperMultithreadedCompressedRoundTrip(fiA, 3, .995);
        SingleFaultInjector last2(0,1);
        helperMultithreadedCompressedRoundTrip(fiA, 2, .985);

        SingleFaultInjector last4(0,3);
        helperMultithreadedCompressedRoundTrip(last4, 4, 1.01);
        SingleFaultInjector last8(0,7);
        helperMultithreadedCompressedRoundTrip(last8, 8, 1.08);
    }


    void testMultithreadedCompressedRoundTripLZHAM( void )
    {
        SingleFaultInjector first(0,0);
        helperMultithreadedCompressedRoundTrip(first, 8, 1.08, true);
        SingleFaultInjector fi(0,1);
        helperMultithreadedCompressedRoundTrip(fi, 4, 1.01, true);

        SingleFaultInjector fiA(1,1);
        helperMultithreadedCompressedRoundTrip(fiA, 3, .995, true);
        SingleFaultInjector last2(0,1);
        helperMultithreadedCompressedRoundTrip(fiA, 2, .985, true);

        SingleFaultInjector last4(0,3);
        helperMultithreadedCompressedRoundTrip(last4, 4, 1.01, true);
        SingleFaultInjector last8(0,7);
        helperMultithreadedCompressedRoundTrip(last8, 8, 1.08, true);
    }

    void helperMultithreadedCompressedRoundTrip( Sirikata::FaultInjectorXZ &fi, int nthreads, double ratio, bool lzham = false)
    {
        using namespace Sirikata;
        Sirikata::JpegAllocator<uint8_t>alloc;
        MemReadWriter original(alloc);
        ThreadContext * tc = TestMakeThreadContext(nthreads, alloc, false, &fi);
        size_t inputDataSize = loadFile("prism/texture0.jpg", original, alloc);
        MemReadWriter arhc(alloc);
        ConstantSizeEstimator size_estimate = inputDataSize;
        JpegError err = MultiCompressAnyto7Z(original, arhc, 6, lzham, &size_estimate, tc);
        if (err != JpegError()) {
            fprintf(stderr, "7Z compression Error: %s\n", err.what());
        }
        TS_ASSERT_EQUALS(err, JpegError());
        MemReadWriter round(alloc);
        err = MultiDecompress7ZtoAny(arhc, round, tc);
        DestroyThreadContext(tc);
        TS_ASSERT_EQUALS(err, JpegError());
        TS_ASSERT_EQUALS(original.buffer(), round.buffer());
        TS_ASSERT_LESS_THAN(arhc.buffer().size(), (size_t)(inputDataSize * ratio));
    }
    void testMultithreadedCompressedIntegration( bool use_lzham ) // makes sure the real functions work
    {
        helperMultithreadedCompressedIntegration(false);
    }
    void testMultithreadedCompressedLZHAMIntegration( bool use_lzham ) // makes sure the real functions work
    {
        helperMultithreadedCompressedIntegration(true);
    }
    void helperMultithreadedCompressedIntegration( bool use_lzham ) // makes sure the real functions work
    {
        using namespace Sirikata;
        pid_t pid;
        int status = 1;
        if ((pid = fork()) == 0) {
            Sirikata::JpegAllocator<uint8_t>alloc;
            MemReadWriter original(alloc);
        alloc.setup_memory_subsystem(1024 * 1024 * 1024,//4294967295,
                                     16,
                                     &BumpAllocatorInit,
                                     &BumpAllocatorMalloc,
                                     &BumpAllocatorFree,
                                     &BumpAllocatorRealloc,
                                     &BumpAllocatorMsize);
            size_t inputDataSize = loadFile("prism/texture0.jpg", original, alloc);
            ConstantSizeEstimator size_estimate = inputDataSize;
            MemReadWriter arhc(alloc);
            ThreadContext * tc = MakeThreadContext(2, alloc);
            if (prctl(PR_SET_SECCOMP, SECCOMP_MODE_STRICT)) {
                syscall(SYS_exit, 1);
            }

            JpegError err = MultiCompressAnyto7Z(original, arhc, 6, use_lzham, &size_estimate, tc);
            if (err != JpegError()) {
                syscall(SYS_exit, 2);
            }
            TS_ASSERT_EQUALS(err, JpegError());
            MemReadWriter round(alloc);
            err = MultiDecompress7ZtoAny(arhc, round, tc);
            DestroyThreadContext(tc);
            if (err != JpegError()) {
                syscall(SYS_exit, 3);
            }
            if (original.buffer() != round.buffer()) {
                syscall(SYS_exit, 4);
            }
            if (arhc.buffer().size() > inputDataSize * (98 * 2 +  1) / 200) {
                syscall(SYS_exit, 5);
            }
            syscall(SYS_exit, 0);
        }
        waitpid(pid, &status, 0);
        TS_ASSERT_EQUALS(0, status);

    }
    void testGenericCompressedRoundTrip( void )
    {
        genericCompressedRoundTripHelper(Sirikata::JpegAllocator<uint8_t>());
    }
    void genericCompressedRoundTripHelper(const Sirikata::JpegAllocator<uint8_t> &alloc)
    {
        using namespace Sirikata;
        MemReadWriter original(alloc);
        size_t inputDataSize = loadFile("prism/texture0.jpg", original, alloc);
        MemReadWriter arhc(alloc);
        JpegError err = CompressAnyto7Z(original, arhc, 9, alloc);
        if (err != JpegError()) {
            fprintf(stderr, "7Z compression Error: %s\n", err.what());
        }
        TS_ASSERT_EQUALS(err, JpegError());
        MemReadWriter round(alloc);
        err = Decompress7ZtoAny(arhc, round, alloc);
        TS_ASSERT_EQUALS(err, JpegError());
        TS_ASSERT_EQUALS(original.buffer(), round.buffer());
        TS_ASSERT_LESS_THAN(arhc.buffer().size(), inputDataSize * 99 / 100);
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
    void testSwitchableCompression (void )
    {
        using namespace Sirikata;
        JpegAllocator<uint8_t> alloc;
        alloc.setup_memory_subsystem(1024 * 1024 * 1024,//4294967295,
                                     16,
                                     &BumpAllocatorInit,
                                     &BumpAllocatorMalloc,
                                     &BumpAllocatorFree,
                                     &BumpAllocatorRealloc,
                                     &BumpAllocatorMsize);

        using namespace Sirikata;
        MemReadWriter original(alloc);
        MemReadWriter intermediate(alloc);
        MemReadWriter roundtrip(alloc);
        size_t inputDataSize = loadFile("bunny.dae", original, alloc);
        Sirikata::SwitchableCompressionWriter<Sirikata::DecoderCompressionWriter> compressor(&intermediate, 6, alloc);
        size_t offset = 0;
        size_t delta = 1;
        size_t deltainc = 171;
        bool turnedon = false;
        bool turnedoff = false;
        size_t compression_enabled_at = 0;
        size_t compression_disabled_at = 0;
        while (true) {
            compressor.Write(&original.buffer()[offset], std::min(delta, original.buffer().size() - offset));
            offset += delta;
            delta += deltainc;
            if (offset > inputDataSize / 3 && !turnedon) {
                compression_enabled_at = offset;
                compressor.EnableCompression();
                turnedon = true;
            } else if (offset > 2 * inputDataSize / 3 && !turnedoff) {
                compression_disabled_at = offset;
                compressor.DisableCompression();
                turnedoff = true;
            }
            if (offset >= original.buffer().size()) {
                break;
            }
        }
        TS_ASSERT(compression_disabled_at);
        TS_ASSERT(compression_enabled_at);
        compressor.Close();
        Sirikata::SwitchableDecompressionReader<Sirikata::SwitchableXZBase> decompressor(&intermediate, alloc);
        offset = 0;
        while (true) {
            unsigned char buffer[4096];
            unsigned int cur_read = decompressor.Read(buffer, std::min(sizeof(buffer),
                                                                       compression_enabled_at - offset)).first;
            roundtrip.Write(buffer, cur_read);
            offset += cur_read;
            if (offset == compression_enabled_at) {
                break;
            }
        }
        decompressor.EnableCompression();
        while (true) {
            unsigned char buffer[4096];
            unsigned int cur_read = decompressor.Read(buffer, std::min(sizeof(buffer),
                                                                       compression_disabled_at - offset)).first;
            roundtrip.Write(buffer, cur_read);
            offset += cur_read;
            if (offset == compression_disabled_at) {
                break;
            }
        }        
        decompressor.DisableCompression();
        while (true) {
            unsigned char buffer[4096];
            unsigned int cur_read = decompressor.Read(buffer, std::min(sizeof(buffer),
                                                                       inputDataSize - offset)).first;
            roundtrip.Write(buffer, cur_read);
            offset += cur_read;
            if (offset == inputDataSize) {
                break;
            }
        }
        TS_ASSERT_EQUALS(original.buffer(), roundtrip.buffer());
        TS_ASSERT_LESS_THAN(intermediate.buffer().size() * 5 / 4, original.buffer().size()); 
        alloc.teardown_memory_subsystem(&BumpAllocatorDestroy);
        
    }
};

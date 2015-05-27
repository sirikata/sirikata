/*  Sirikata Tests -- Sirikata Test Suite
 *  AnyTest.hpp
 *
 *  Copyright (c) 2009, Daniel Reiter Horn
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
#include <sirikata/core/util/Paths.hpp>
#include <sirikata/core/jpeg-arhc/Compression.hpp>
#include <sirikata/core/jpeg-arhc/BumpAllocator.hpp>
#ifdef __linux
#include <sys/wait.h>
#include <linux/seccomp.h>

#include <sys/prctl.h>
#include <sys/syscall.h>
#endif

class LosslessJpegTest : public CxxTest::TestSuite
{
    class FileReader : public Sirikata::DecoderReader {
        FILE * fp;
        Sirikata::JpegAllocator<char> mAllocator;
    public:
        FileReader(FILE * ff, const Sirikata::JpegAllocator<char> &alloc) : mAllocator(alloc) {
            fp = ff;
        }
        std::pair<Sirikata::uint32, Sirikata::JpegError> Read(Sirikata::uint8*data, unsigned int size) {
            using namespace Sirikata;
            signed long nread = fread(data, 1, size, fp);
            if (nread <= 0) {
                return std::pair<Sirikata::uint32, JpegError>(0, JpegError::errEOF());
            }
            return std::pair<Sirikata::uint32, JpegError>(nread, JpegError());
        }
    };
    class FileWriter : public Sirikata::DecoderWriter {
        FILE * fp;
        Sirikata::JpegAllocator<char> mAllocator;
    public:
        FileWriter(FILE * ff, const Sirikata::JpegAllocator<char> &alloc) : mAllocator(alloc) {
            fp = ff;
        }
        std::pair<Sirikata::uint32, Sirikata::JpegError> Write(const Sirikata::uint8*data, unsigned int size) {
            using namespace Sirikata;
            signed long nwritten = fwrite(data, size, 1, fp);
            if (nwritten == 0) {
                return std::pair<Sirikata::uint32, JpegError>(0, MakeJpegError("Short write"));
            }
            return std::pair<Sirikata::uint32, JpegError>(size, JpegError());
        }
        void Close() {
            fclose(fp);
            fp = NULL;
        }
    };
    

public:
    
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
    void testRoundTrip( void )
    {
        using namespace Sirikata;
        JpegAllocator<uint8_t> alloc;
        MemReadWriter original(alloc);
        loadFile("prism/texture0.jpg", original, alloc);
        uint8 componentCoalescing = Decoder::comp12coalesce;
        MemReadWriter arhc(alloc);
        JpegError err = Decode(original, arhc, componentCoalescing, alloc);
        TS_ASSERT_EQUALS(err, JpegError());
        MemReadWriter round(alloc);
        err = Decode(arhc, round, componentCoalescing, alloc);
        TS_ASSERT_EQUALS(err, JpegError());
        TS_ASSERT_EQUALS(original.buffer(), round.buffer());
    }
    void testBumpAllocator( void )
    {
        using namespace Sirikata;
        TS_ASSERT_EQUALS(BumpAllocatorTest(), true);
    }
    void testCompressedRoundTrip( void )
    {
        using namespace Sirikata;
        JpegAllocator<uint8_t> alloc;
        compressedRoundTripHelper(alloc);
    }
    void testSeccompRoundTrip (void) {
        pid_t pid;
        int status = 1;
        if ((pid = fork()) == 0) {
            using namespace Sirikata;
            JpegAllocator<uint8_t> alloc;
            alloc.setup_memory_subsystem(768 * 1024 * 1024,
                                         1,
                                         &BumpAllocatorInit,
                                         &BumpAllocatorMalloc,
                                         &BumpAllocatorFree,
                                         &BumpAllocatorRealloc,
                                         &BumpAllocatorMsize);
            
            std::pair<FILE*, size_t> input_fp_size = getFileObjectAndSize("prism/texture0.jpg");
            if (prctl(PR_SET_SECCOMP, SECCOMP_MODE_STRICT)) {
                syscall(SYS_exit, 1); // SECCOMP not allowed
            }
            
            MemReadWriter original(alloc);
            size_t inputDataSize = loadFileHelper(input_fp_size.first, input_fp_size.second, original, alloc);

            uint8 componentCoalescing = Decoder::comp12coalesce;
            MemReadWriter arhc(alloc);
            JpegError err = CompressJPEGtoARHC(original, arhc, 9, componentCoalescing, alloc);
            if (err != JpegError()) {
                syscall(SYS_exit, 2);
            }
            MemReadWriter round(alloc);
            err = DecompressARHCtoJPEG(arhc, round, alloc);
            if (err != JpegError()) {
                syscall(SYS_exit, 3);
            }
            if (original.buffer() != round.buffer()) {
                syscall(SYS_exit, 4);
            }
            if (arhc.buffer().size() > inputDataSize * 9 / 10) {
                syscall(SYS_exit, 5);
            }
            syscall(SYS_exit, 0);
        }
        waitpid(pid, &status, 0);
        TS_ASSERT_EQUALS(0, status);
    }


    void testMultiSeccompARHC (void) {
        pid_t pid;
        int status = 1;
        if ((pid = fork()) == 0) {
            using namespace Sirikata;
            JpegAllocator<uint8_t> alloc;
            alloc.setup_memory_subsystem(2147483647,//4294967295,
                                         16,
                                         &BumpAllocatorInit,
                                         &BumpAllocatorMalloc,
                                         &BumpAllocatorFree,
                                         &BumpAllocatorRealloc,
                                         &BumpAllocatorMsize);
            
            std::pair<FILE*, size_t> input_fp_size = getFileObjectAndSize("prism/texture0.jpg");
            ThreadContext * tc = MakeThreadContext(3, alloc);            
            if (prctl(PR_SET_SECCOMP, SECCOMP_MODE_STRICT)) {
                syscall(SYS_exit, 1); // SECCOMP not allowed
            }
            MemReadWriter original(alloc);
            size_t inputDataSize = loadFileHelper(input_fp_size.first, input_fp_size.second, original, alloc);
            
            uint8 componentCoalescing = Decoder::comp12coalesce;
            MemReadWriter arhc(alloc);
            JpegError err = CompressJPEGtoARHCMulti(original, arhc, 6, componentCoalescing, tc);
            if (err != JpegError()) {
                syscall(SYS_exit, 2);
            }
            MemReadWriter round(alloc);
            err = DecompressARHCtoJPEGMulti(arhc, round, tc);
            DestroyThreadContext(tc);
            if (err != JpegError()) {
                syscall(SYS_exit, 3);
            }
            if (original.buffer() != round.buffer()) {
                syscall(SYS_exit, 4);
            }
            if (arhc.buffer().size() > inputDataSize * 94 / 100) {
                syscall(SYS_exit, 5);
            }
            syscall(SYS_exit, 0);
       }
       waitpid(pid, &status, 0);
       TS_ASSERT_EQUALS(0, status);
    }

    
    void compressedRoundTripHelper(const Sirikata::JpegAllocator<uint8_t> &alloc) {
        using namespace Sirikata;
        MemReadWriter original(alloc);
        size_t inputDataSize = loadFile("prism/texture0.jpg", original, alloc);
        uint8 componentCoalescing = Decoder::comp12coalesce;
        MemReadWriter arhc(alloc);
        JpegError err = CompressJPEGtoARHC(original, arhc, 9, componentCoalescing, alloc);
        TS_ASSERT_EQUALS(err, JpegError());
        MemReadWriter round(alloc);
        err = DecompressARHCtoJPEG(arhc, round, alloc);
        TS_ASSERT_EQUALS(err, JpegError());
        TS_ASSERT_EQUALS(original.buffer(), round.buffer());
        TS_ASSERT_LESS_THAN(arhc.buffer().size(), inputDataSize * 9 / 10);
    }

    void testCompressedRoundTripBumpAllocator( void )
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
        compressedRoundTripHelper(alloc);
        alloc.teardown_memory_subsystem(&BumpAllocatorDestroy);
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
    void testMultithreadedCompressedIntegration( void ) // makes sure the real functions work
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

            JpegError err = MultiCompressAnyto7Z(original, arhc, 6, &size_estimate, tc);
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
    void helperMultithreadedCompressedRoundTrip( Sirikata::FaultInjectorXZ &fi, int nthreads, double ratio)
    {
        using namespace Sirikata;
        Sirikata::JpegAllocator<uint8_t>alloc;
        MemReadWriter original(alloc);
        ThreadContext * tc = TestMakeThreadContext(nthreads, alloc, false, &fi);
        size_t inputDataSize = loadFile("prism/texture0.jpg", original, alloc);
        MemReadWriter arhc(alloc);
        ConstantSizeEstimator size_estimate = inputDataSize;
        JpegError err = MultiCompressAnyto7Z(original, arhc, 6, &size_estimate, tc);
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
};

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
#include <sirikata/core/jpeg-arhc/Decoder.hpp>
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
                return std::pair<Sirikata::uint32, JpegError>(0, JpegError::errEOF(mAllocator));
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
                return std::pair<Sirikata::uint32, JpegError>(0, JpegError("Short write", mAllocator));
            }
            return std::pair<Sirikata::uint32, JpegError>(size, JpegError());
        }
        void Close() {
            fclose(fp);
            fp = NULL;
        }
    };
    

public:
    size_t loadFile(const char *fileName, Sirikata::MemReadWriter&original,
                    const Sirikata::JpegAllocator<uint8_t> &alloc) {
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
        fseek(input, 0, SEEK_END);
        std::vector<uint8, JpegAllocator<uint8> > inputData(alloc);
        inputData.resize(ftell(input));
        fseek(input, 0, SEEK_SET);
        if (!inputData.empty()) {
            fread(&inputData[0], inputData.size(), 1, input);
            original.Write(&inputData[0], inputData.size());
        }
        fclose(input);
        return inputData.size();
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
    void testCompressedRoundTrip( void )
    {
        using namespace Sirikata;
        JpegAllocator<uint8_t> alloc;
        MemReadWriter original(alloc);
        size_t inputDataSize = loadFile("prism/texture0.jpg", original, alloc);
        uint8 componentCoalescing = Decoder::comp12coalesce;
        MemReadWriter arhc(alloc);
        JpegError err = CompressJPEGtoARHC(original, arhc, componentCoalescing, alloc);
        TS_ASSERT_EQUALS(err, JpegError());
        MemReadWriter round(alloc);
        err = DecompressARHCtoJPEG(arhc, round, alloc);
        TS_ASSERT_EQUALS(err, JpegError());
        TS_ASSERT_EQUALS(original.buffer(), round.buffer());
        TS_ASSERT_LESS_THAN(arhc.buffer().size(), inputDataSize * 9 / 10);
    }
    void testGenericCompressedRoundTrip( void )
    {
        using namespace Sirikata;
        JpegAllocator<uint8_t> alloc;
        MemReadWriter original(alloc);
        size_t inputDataSize = loadFile("prism/texture0.jpg", original, alloc);
        MemReadWriter arhc(alloc);
        JpegError err = CompressAnyto7Z(original, arhc, alloc);
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
};

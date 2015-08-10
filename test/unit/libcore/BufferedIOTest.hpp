#include <cxxtest/TestSuite.h>
#include <memory>
#include <cstring>
#include <sirikata/core/util/Paths.hpp>
#include <sirikata/core/jpeg-arhc/BumpAllocator.hpp>
#include <sirikata/core/jpeg-arhc/BufferedIO.hpp>

class BufferedIOTest : public CxxTest::TestSuite
{
public:
    void testBufferedIO( void ) {
        using namespace Sirikata;
        uint32_t testSize = 1024 * 1024 * 131;
        std::vector<uint8_t> data(testSize);
        for (size_t i = 0; i < testSize; ++i) {
            data[i] = rand() & 0xff;
        }
        JpegAllocator<uint8_t> alloc;
        MemReadWriter mem(alloc);
        BufferedWriter<65536> wri(&mem);
        uint32_t progress = 0;
        uint32_t writeSize = 1;
        uint32_t writeJump = 65529;
        uint32_t writeMo = 255053;
        while (progress < testSize) {
            uint32_t toWrite = std::min(writeSize, testSize - progress);
            JpegError err = wri.Write(&data[progress], toWrite).second;
            TS_ASSERT(err == JpegError::nil());
            progress += toWrite;
            writeSize += writeJump;
            writeSize %= writeMo;
        }
        wri.Close();
        TS_ASSERT_EQUALS(mem.buffer().size(), data.size());
        TS_ASSERT_EQUALS(memcmp(&mem.buffer()[0], &data[0], data.size()), 0);
        BufferedReader<4096> rea(&mem);
        progress = 0;
        memset(&data[0], 0x7c, testSize);
        while (progress < testSize) {
            uint32_t toRead = std::min(writeSize, testSize - progress);
            std::pair<uint32_t, JpegError> err = rea.Read(&data[progress], toRead);
            toRead = err.first;
            TS_ASSERT(err.second == JpegError::nil());
            progress += toRead;
            writeSize += writeJump;
            writeSize %= writeMo;
        }
        TS_ASSERT_EQUALS(mem.buffer().size(), data.size());
        TS_ASSERT_EQUALS(memcmp(&mem.buffer()[0], &data[0], data.size()), 0);
    }

};

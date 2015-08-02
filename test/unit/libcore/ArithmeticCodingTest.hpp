#include <cxxtest/TestSuite.h>
#include <memory>
#include <cstring>
#include <sirikata/core/jpeg-arhc/ArithmeticCoder.hpp>

class ArithmeticCodingTest : public CxxTest::TestSuite
{
public:
    void testRoundTrip() {
        using namespace Sirikata;
        Sirikata::JpegAllocator<uint8_t> alloc;
        std::auto_ptr<MemReadWriter> mem_buffer(new MemReadWriter(alloc));
        unsigned char bitbuffer[1024] = {1,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
                                         0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
                                         0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
                                         0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,

                                         1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1,
                                         1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1,
                                         1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1,
                                         1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1,

                                         1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1,
                                         1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1,
                                         1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1,
                                         1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1,

                                         1,1,1,0, 1,1,1,0, 1,1,1,0, 1,1,1,0,
                                         1,1,1,0, 1,1,1,0, 1,1,1,0, 1,1,1,0,
                                         1,1,1,0, 1,1,1,0, 1,1,1,0, 1,1,1,0,
                                         1,1,1,0, 1,1,1,0, 1,1,1,0, 1,1,1,0,

                                         1,1,1,0, 1,1,1,0, 1,1,1,0, 1,1,1,0,
                                         1,1,1,0, 1,1,1,0, 1,1,1,0, 1,1,1,0,
                                         1,1,1,0, 1,1,1,0, 1,1,1,0, 1,1,1,0,
                                         1,1,1,0, 1,1,1,0, 1,1,1,0, 1,1,1,0,

                                         0,0,0,1, 0,0,0,1, 0,0,0,1, 0,0,0,1,
                                         0,0,0,1, 0,0,0,1, 0,0,0,1, 0,0,0,1,
                                         0,0,0,1, 0,0,0,1, 0,0,0,1, 0,0,0,1,
                                         0,0,0,1, 0,0,0,1, 0,0,0,1, 0,0,0,1,

                                         0,0,0,1, 0,0,0,1, 0,0,0,1, 0,0,0,1,
                                         0,0,0,1, 0,0,0,1, 0,0,0,1, 0,0,0,1,
                                         0,0,0,1, 0,0,0,1, 0,0,0,1, 0,0,0,1,
                                         0,0,0,1, 0,0,0,1, 0,0,0,1, 0,0,0,1,

                                         0,0,0,1, 0,0,0,1, 0,0,0,1, 0,0,0,1,
                                         0,0,0,1, 0,0,0,1, 0,0,0,1, 0,0,0,1,
                                         0,0,0,1, 0,0,0,1, 0,0,0,1, 0,0,0,1,
                                         0,0,0,1, 0,0,0,1, 0,0,0,1, 0,0,0,1};

        unsigned char readbuffer[sizeof(bitbuffer)];
        unsigned char original_state = 0;
        unsigned char state = original_state;
        ArithmeticWriter coding(mem_buffer.get());
        for (size_t i = 0; i < sizeof(bitbuffer); ++i) {
            coding.WriteBit(&state, bitbuffer[i]);
        }
        coding.Finish();

        state = original_state;
        ArithmeticReader decoding(mem_buffer.get());
        for (size_t i = 0; i < sizeof(bitbuffer); ++i) {
            readbuffer[i] = decoding.ReadBit(&state);
        }
        int equality = std::memcmp(readbuffer, bitbuffer, sizeof(bitbuffer));
        TS_ASSERT_EQUALS(equality, 0);
        TS_ASSERT_EQUALS(std::vector<uint8_t>(bitbuffer,bitbuffer + sizeof(bitbuffer)),
                         std::vector<uint8_t>(readbuffer,readbuffer + sizeof(readbuffer)));
        if (equality != 0) {
            for (int i = 0; i < 32; ++i) {
                for (size_t j = 0; j < sizeof(bitbuffer)/32; ++j) {
                    fprintf(stderr, "%d ", bitbuffer[i * sizeof(bitbuffer)/32 + j]);
                }
                fprintf(stderr, "\nvs\n");
                for (size_t j = 0; j < sizeof(bitbuffer)/32; ++j) {
                    fprintf(stderr, "%d ", readbuffer[i * sizeof(bitbuffer)/32 + j]);
                }
                fprintf(stderr, "\n\n");
            }
            fprintf(stderr, "Mem buffer was %ld bytes\n", mem_buffer->buffer().size());
        }
    }
    void testEmptyRoundTrip() {
        using namespace Sirikata;
        Sirikata::JpegAllocator<uint8_t> alloc;
        std::auto_ptr<MemReadWriter> mem_buffer(new MemReadWriter(alloc));
        unsigned char bitbuffer[1024]={};
        unsigned char readbuffer[sizeof(bitbuffer)];
        unsigned char original_state = 0;
        unsigned char state = original_state;
        ArithmeticWriter coding(mem_buffer.get());
        for (size_t i = 0; i < sizeof(bitbuffer); ++i) {
            coding.WriteBit(&state, bitbuffer[i]);
        }
        coding.Finish();

        state = original_state;
        ArithmeticReader decoding(mem_buffer.get());
        for (size_t i = 0; i < sizeof(bitbuffer); ++i) {
            readbuffer[i] = decoding.ReadBit(&state);
        }
        int equality = std::memcmp(readbuffer, bitbuffer, sizeof(bitbuffer));
        TS_ASSERT_EQUALS(equality, 0);
        TS_ASSERT_EQUALS(std::vector<uint8_t>(bitbuffer,bitbuffer + sizeof(bitbuffer)),
                         std::vector<uint8_t>(readbuffer,readbuffer + sizeof(readbuffer)));
        if (equality != 0) {
            for (int i = 0; i < 32; ++i) {
                for (size_t j = 0; j < sizeof(bitbuffer)/32; ++j) {
                    fprintf(stderr, "%d ", bitbuffer[i * sizeof(bitbuffer)/32 + j]);
                }
                fprintf(stderr, "\nvs\n");
                for (size_t j = 0; j < sizeof(bitbuffer)/32; ++j) {
                    fprintf(stderr, "%d ", readbuffer[i * sizeof(bitbuffer)/32 + j]);
                }
                fprintf(stderr, "\n\n");
            }
            fprintf(stderr, "Mem buffer was %ld bytes\n", mem_buffer->buffer().size());
        }
    }
};

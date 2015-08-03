#include <sirikata/core/jpeg-arhc/MuxReader.hpp>

class MuxReadWriterTest : public CxxTest::TestSuite
{
public:

    void setupTestData(std::vector<uint8_t> *testData, size_t num_items) {
        for(size_t j = 0; j < num_items; ++j) {
            for (size_t i = 0; i + 3 < testData[j].size(); i += 4) {
                switch(j) {
                  case 0:
                    testData[j][i] = i & 0xff;
                    testData[j][i+1] = (1 ^ (i >> 8)) & 0xff;
                    testData[j][i+2] = (2 ^ (i >> 16)) & 0xff;
                    testData[j][i+3] = (3 ^ (i >> 24)) & 0xff;
                    break;
                  case 1:
                    testData[j][i] = testData[0][i%testData[0].size()]
                        ^ testData[0][(i+1)%testData[0].size()];
                    testData[j][i + 1] = testData[0][(i + 1)%testData[0].size()]
                        ^ testData[0][(i+2)%testData[0].size()];
                    testData[j][i + 2 ] = testData[0][(i + 2)%testData[0].size()]
                        ^ testData[0][(i+3)%testData[0].size()];
                    testData[j][i + 3 ] = testData[0][(i + 3)%testData[0].size()]
                        ^ testData[0][(i+0)%testData[0].size()];
                    break;
                  default:
                    testData[j][i] = (j ^ (i + 1) ^ 0xf89721) & 0xff;
                    testData[j][i] = ((j ^ (i + 1) ^ 0xf89721) >> 8) & 0xff;
                    testData[j][i] = ((j ^ (i + 1) ^ 0xf89721) >> 16) & 0xff;
                    testData[j][i] = ((j ^ (i + 1) ^ 0xf89721) >> 24) & 0xff;
                }
            }
        }
    }
    
    void testEof() {
        std::vector<uint8_t> testData[4];
        uint32_t progress[sizeof(testData) / sizeof(testData[0])] = {0};
        const uint32_t a0 = 2, a1 = 257, b0 = 2, b1 = 4097,
            c0 = 256, c1 = 16385, d0 = 2, d1 = 32768, e0 = 256, e1 = 65537, f1 = 65536;
        const uint32_t maxtesty = a1 + b1 + c1 + d1 + e1 + f1;
        testData[0].resize(256);
        testData[1].resize(maxtesty);
        setupTestData(testData, sizeof(testData)/sizeof(testData[0]));
        using namespace Sirikata;
        JpegAllocator<uint8_t> alloc;
        MemReadWriter rw(alloc);
        MuxReader reader(&rw, alloc);
        MuxWriter writer(&rw, alloc);
        writer.Write(1, &testData[1][0], a1);
        writer.Write(0, &testData[0][0], a0);
        writer.Write(1, &testData[1][a1], b1);
        writer.Write(0, &testData[0][a0], b0);
        writer.Write(1, &testData[1][a1 + b1], c1);
        writer.Write(0, &testData[0][a0 + b0], c0);        
        writer.Write(1, &testData[1][a1 + b1 + c1], d1);
        writer.Write(0, &testData[0][a0 + b0 + c0], d0);
        writer.Write(1, &testData[1][a1 + b1 + c1 + d1], e1);
        writer.Write(0, &testData[0][a0 + b0 + c0 + d0], e0);
        writer.Write(1, &testData[1][a1 + b1 + c1 + d1+ e1 ], f1);
        writer.Close();
        uint8_t testx[260];
        uint32_t offset = 0;
        while(offset<sizeof(testx)) {
            std::pair<uint32_t, JpegError> r = reader.Read(0, testx + offset, sizeof(testx) - offset);
            TS_ASSERT(r.first > 0);
            TS_ASSERT_EQUALS(r.second, JpegError::nil());
            offset += r.first;
            if (!r.first) {
                return; // ERROR IN TEST;
            }
        }
        TS_ASSERT_EQUALS(memcmp(testx, &testData[0][0], sizeof(testx)), 0);
        offset = 0;
        std::pair<uint32_t, JpegError> expectEof(0, JpegError::nil());
        while(offset<sizeof(testx)) {
            expectEof = reader.Read(0, testx + offset, sizeof(testx) - offset);
            offset += expectEof.first;
            if (expectEof.second != JpegError::nil()) {
                break;
            }
        }
        TS_ASSERT_EQUALS(offset, 258);
        TS_ASSERT_EQUALS(expectEof.second, JpegError::errEOF());

        uint8_t testy[maxtesty];
        offset = 0;
        while(offset<sizeof(testy)) {
            std::pair<uint32_t, JpegError> r = reader.Read(1, testy + offset, sizeof(testy) - offset);
            TS_ASSERT(r.first > 0);
            TS_ASSERT_EQUALS(r.second, JpegError::nil());
            offset += r.first;
            if (r.second != JpegError::nil() || !r.first) {
                return; // ERROR IN TEST;
            }
        }
        int res = memcmp(testy, &testData[1][0], sizeof(testy));
        if (res) {
            for (size_t i = 0; i < sizeof(testy); ++i) {
                if (testy[i] != testData[1][i]) {
                    fprintf(stderr, "data[%d]:: %x != %x\n", (int)i, testy[i], testData[1][i]);
                }
            }
        }
        TS_ASSERT_EQUALS(res, 0);

    }
    void testRoundtrip() {
        std::vector<uint8_t> testData[4];
        std::vector<uint8_t> roundTrip[sizeof(testData) / sizeof(testData[0])];
        uint32_t progress[sizeof(testData) / sizeof(testData[0])] = {0};
        int invProb[sizeof(testData) / sizeof(testData[0])];
        for (size_t j = 0; j < sizeof(testData) / sizeof(testData[0]); ++j) {
            testData[j].resize(131072 * 4 + 1);
            invProb[j] = j + 2;
        }
        testData[3].resize(testData[3].size() + 1);
        memset(&testData[3][0], 0xef, testData[3].size());
        testData[3].back() = 0x47;
        
        setupTestData(testData, sizeof(testData)/ sizeof(testData[0]));
        using namespace Sirikata;
        JpegAllocator<uint8_t> alloc;
        MemReadWriter rw(alloc);
        MuxReader reader(&rw, alloc);
        MuxWriter writer(&rw, alloc);
        srand(1023);
        
        bool allDone;
        do {
            allDone = true;
            for(size_t j = 0; j < sizeof(testData) / sizeof(testData[0]); ++j) {
                if (progress[j] < testData[j].size()) {
                    if (rand() < RAND_MAX / invProb[j]) {
                        size_t bytesLeft = testData[j].size() - progress[j];
                        size_t desiredWrite = 1;
                        if (j >= 0 && rand() < rand()/3) {
                            desiredWrite = rand()%5;
                        }
                        if (j >= 0 && rand() < rand()/8) {
                            desiredWrite = rand()%256;
                        }

                        if (j >= 1 && rand() < rand()/32) {
                            desiredWrite = 4096;
                        }
                        if (j >= 2 && rand() < rand()/64) {
                            desiredWrite = 16384;
                        }

                        if (j == 3 && rand() < rand()/128) {
                            desiredWrite = 131073;
                        }
                        uint32_t amountToWrite = std::min(desiredWrite,
                                                          testData[j].size() - progress[j]);
                        
                        JpegError err = writer.Write(j,
                                                     &testData[j][progress[j]],
                                                     amountToWrite).second;
                        progress[j] += amountToWrite;
                        TS_ASSERT(err == JpegError::nil());
                    }
                    allDone = false;
                }
            }
        }while(!allDone);
        writer.Close();
        for(size_t j = 0; j < sizeof(testData) / sizeof(testData[0]); ++j) {
            assert(progress[j] == testData[j].size());
            progress[j] = 0;
            roundTrip[j].resize(testData[j].size());
        }
        do {
            allDone = true;
            for(size_t j = 0; j < sizeof(testData) / sizeof(testData[0]); ++j) {
                if (progress[j] < testData[j].size()) {
                    if (rand() < RAND_MAX / invProb[j]) {
                        size_t bytesLeft = testData[j].size() - progress[j];
                        size_t desiredRead = 1;
                        if (j >= 0 && rand() < rand()/3) {
                            desiredRead = rand()%5;
                        }
                        if (j >= 0 && rand() < rand()/8) {
                            desiredRead = rand()%256;
                        }

                        if (j >= 1 && rand() < rand()/32) {
                            desiredRead = 4096;
                        }
                        if (j >= 2 && rand() < rand()/64) {
                            desiredRead = 16384;
                        }

                        if (j == 3 && rand() < rand()/128) {
                            desiredRead = 131073;
                        }
                        uint32_t amountToRead = std::min(desiredRead,
                                                          testData[j].size() - progress[j]);
                        
                        std::pair<uint32_t,JpegError> ret = reader.Read(j,
                                                                        &roundTrip[j][progress[j]],
                                                                        amountToRead);
                        for (uint32_t r = 0; r < ret.second; ++r) {
                            TS_ASSERT_EQUALS(roundTrip[j][progress[j] + r],
                                             testData[j][progress[j] + r]);
                        }
                        progress[j] += ret.first;
                        TS_ASSERT(ret.second == JpegError::nil());
                    }
                    allDone = false;
                }
            }
        }while(!allDone);
        for(size_t j = 0; j < sizeof(testData) / sizeof(testData[0]); ++j) {
            TS_ASSERT_EQUALS(roundTrip[j],
                             testData[j]);
        }
    }
};



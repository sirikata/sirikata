#include <sirikata/core/util/ArrayNd.hpp>

class ArrayNdTest : public CxxTest::TestSuite
{
    static void podtest(int i,...) {
        (void)i;
    }
public:
    void testSetAt() {
        using namespace Sirikata;
        AlignedArray7d<unsigned char, 1,3,2,5,4,6,16> aligned7d;
        uint8_t* d =&aligned7d.at(0, 2, 1, 3, 2, 1, 0);
        *d = 4;
        size_t offset = d - (uint8_t*)0;
        assert(0 == (offset & 15) && "Must have alignment");
        assert(aligned7d.at(0, 2, 1, 3, 2, 1, 0) == 4);
        Array7d<unsigned char, 1,3,2,5,3,3,16> a7;
        uint8_t* d2 =&a7.at(0, 2, 1, 3, 2, 1, 0);
        *d2 = 5;
        offset = d2 - (uint8_t*)0;
        if (offset & 15) {
            fprintf(stderr, "Array7d array doesn't require alignment");
        }
        assert(a7.at(0, 2, 1, 3, 2, 1, 0) == 5);
        a7.at(0, 2, 1, 3, 2, 1, 1) = 8;
        assert(a7.at(0, 2, 1, 3, 2, 1, 1) == 8);
        Slice1d<unsigned char, 16> s = a7.at(0, 2, 1, 3, 2, 1);
        s.at(1) = 16;
        assert(a7.at(0, 2, 1, 3, 2, 1, 1) == 16);
        s.at(0) = 6;
        assert(a7.at(0, 2, 1, 3, 2, 1, 0) == 6);
        {
            a7.at(0,0,0,0,0,0,0) = 16;
            assert(a7.at(0,0,0,0,0,0,0) == a7.at(0).at(0).at(0).at(0).at(0).at(0).at(0));
            a7.at(0).at(0).at(0).at(0).at(0).at(0).at(0) = 47;
            assert(a7.at(0,0,0,0,0,0,0) == 47);
        }
        podtest(4, a7);
        
    }

};


#include <sirikata/core/util/ArrayNd.hpp>

class ArrayNdTest : public CxxTest::TestSuite
{
    static void podtest(int i,...) {
        (void)i;
    }
public:
    struct SetNeg5 {
        void operator()(int &i) const{
            i = 5;
        }
    };
    struct Sum {
        mutable uint64_t total;
        Sum() {
            total = 0;
        }
        void operator()(const int &i) const{
            total += i;
        }
    };
    void testMemset() {
        using namespace Sirikata;
        Array7d<int, 1,3,2,5,4,6,16> a7d;

        a7d.foreach(SetNeg5());
        assert(a7d.at(0, 2, 1, 3, 2, 0, 0) == 5);
        assert(a7d.at(0, 2, 1, 3, 2, 1, 0) == 5);
        a7d.at(0, 2, 1, 3, 2, 1).memset(0x7f);
        assert(a7d.at(0, 2, 1, 3, 2, 1, 0) == 0x7f7f7f7f);
        assert(a7d.at(0, 2, 1, 3, 2, 1, 1) == 0x7f7f7f7f);
        assert(a7d.at(0, 2, 1, 3, 2, 1, 15) == 0x7f7f7f7f);
        assert(a7d.at(0, 2, 1, 3, 2, 0, 0) == 5); // check it only affected the slice
        Sum s;
        a7d.foreach(s);
        assert(s.total == 34225051808ULL);
    }

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
        assert(a7.size()[0] == 1);
        assert(a7.size()[1] == 3);
        assert(a7.size()[6] == 16);
        podtest(4, a7);
        
    }

};


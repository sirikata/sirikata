/*  Sirikata Jpeg Texture Transfer
 *  MemMgrAllocator Unit Tests
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
#include <sirikata/core/jpeg-arhc/MemMgrAllocator.hpp>
#if __cplusplus <= 199711L
#include <sirikata/core/util/Thread.hpp>
#else
#include <thread>
#endif
class MemMgrAllocatorTest : public CxxTest::TestSuite
{
    void * wrapped_alloc(size_t size) {
        using namespace Sirikata;
        TS_ASSERT_EQUALS((size & (sizeof(void*) - 1)), 0);
        void ** retval = (void**)memmgr_alloc(size);
        size/=sizeof(void*);
        for (size_t i= 0; i < size;++i) {
            retval[i] = retval;
        }
        return retval;
    }
    void wrapped_free(void * data, size_t size) {
        using namespace Sirikata;
        void ** test = (void**)data;
        size /= sizeof(void*);
        for (size_t i = 0; i < size; ++i) {
            TS_ASSERT_EQUALS(test[i], data); // make sure data hasn't been corrutped
        }
        memmgr_free(data);
    }
    int integration() {
        using namespace Sirikata;
        std::map<char *, size_t> *allocated = NULL;

        allocated = new std::map<char *, size_t>;
        srand(10420);
        for (size_t i = 0; i < 16384; ++i) {
            size_t to_allocate = (rand() % 256 + 1) * sizeof(void*);
            (*allocated)[(char*)wrapped_alloc(to_allocate)] = to_allocate;
            if ((i & 3) == 3) {
                if ((*allocated).size()) {
                    char* lowest = allocated->begin()->first;
                    std::map<char *, size_t>::iterator last = allocated->end();
                    --last;
                    char * highest = last->first;
                    size_t delta = highest - lowest;
                    std::map<char *, size_t>::iterator mfd = last;
                    if (delta !=0) {
                        mfd = allocated->lower_bound(lowest + rand()% delta);
                        if (mfd == allocated->end()) {
                            --mfd;
                        }
                    }
                    wrapped_free(mfd->first, mfd->second);
                    allocated->erase(mfd);
                }
            }
        }
        
        memmgr_free(allocated); // test what happens if you free something out of bounds
        while(!allocated->empty()) { //clear the remainder
            wrapped_free(allocated->begin()->first, allocated->begin()->second);
            allocated->erase(allocated->begin());
        }
        delete allocated;
        return 0;
    }
    void unit() {
        using namespace Sirikata;
        uint8_t *p[30] = {0};
        int i;
        // Each header uses 8 bytes, so this allocates
        // 3 * (2048 + 8) = 6168 bytes, leaving us
        // with 8192 - 6168 = 2024
        //
        for (i = 0; i < 3; ++i)
        {
            p[i] = (uint8_t*)memmgr_alloc(2048);
            TS_ASSERT(p[i]);
        }
        
        // Allocate all the remaining memory
        //
        p[4] = (uint8_t*)memmgr_alloc(sizeof(void*) == 4 ? 2016 : 1984);
        TS_ASSERT(p[4]);
        
        // Nothing left...
        //
        p[5] = (uint8_t*)memmgr_alloc(1);
        TS_ASSERT(p[5] == 0);
        
        // Release the second block. This frees 2048 + 8 bytes.
        //
        memmgr_free(p[1]);
        p[1] = 0;

        // Now we can allocate several smaller chunks from the
        // free list. There, they can be smaller than the
        // minimal allocation size.
        // Allocations of 100 require 14 quantas (13 for the
        // requested space, 1 for the header). So it allocates
        // 112 bytes. We have 18 allocations to make:
        //
        for (i = 10; i < (sizeof(void*) == 4 ? 28 : 26); ++i)
        {
            p[i] = (byte*)memmgr_alloc(100);
            TS_ASSERT(p[i]);
        }
        
        // Not enough for another one...
        //
        p[28] = (byte*)memmgr_alloc(100);
        TS_ASSERT(p[28] == 0);
        
        // Now free everything
        //
        for (i = 0; i < 30; ++i)
        {
            if (p[i])
                memmgr_free(p[i]);
        }
        
        memmgr_print_stats();
    }
public:
    void testSimpleIntegration( void ) {
        using namespace Sirikata;
        memmgr_init(128 * 1024 * 1024, 0, 0);
        int retval = integration();
        if (retval == 0) {
            printf("OK\n");
        }
        TS_ASSERT_EQUALS(retval, 0);
        memmgr_destroy();
        memmgr_destroy(); // make sure this is idempotent
        memmgr_destroy();
    }
    void testIntegration( void ) {
        using namespace Sirikata;
        memmgr_init(128 * 1024 * 1024, 128 * 1024 * 1024, 4);
#if __cplusplus <= 199711L
        Thread a("A", std::tr1::bind(&MemMgrAllocatorTest::integration, this));
        Thread b("B", std::tr1::bind(&MemMgrAllocatorTest::integration, this));
        Thread c("C", std::tr1::bind(&MemMgrAllocatorTest::integration, this));
        Thread d("D", std::tr1::bind(&MemMgrAllocatorTest::integration, this));
#else
        std::thread a(std::bind(&MemMgrAllocatorTest::integration, this));
        std::thread b(std::bind(&MemMgrAllocatorTest::integration, this));
        std::thread c(std::bind(&MemMgrAllocatorTest::integration, this));
        std::thread d(std::bind(&MemMgrAllocatorTest::integration, this));
#endif
        int retval = integration();
        a.join();
        b.join();
        c.join();
        d.join();
        if (retval == 0) {
            printf("OK\n");
        }
        TS_ASSERT_EQUALS(retval, 0);
        memmgr_destroy();
    }
    void testUnit ( void ) {
        using namespace Sirikata;
        memmgr_init(8 * 1024, 8 * 1024, 4, 16);
#if __cplusplus <= 199711L
        Thread a("A", std::tr1::bind(&MemMgrAllocatorTest::unit, this));
        Thread b("B", std::tr1::bind(&MemMgrAllocatorTest::unit, this));
        Thread c("C", std::tr1::bind(&MemMgrAllocatorTest::unit, this));
        Thread d("D", std::tr1::bind(&MemMgrAllocatorTest::unit, this));
#else
        std::thread a(std::bind(&MemMgrAllocatorTest::unit, this));
        std::thread b(std::bind(&MemMgrAllocatorTest::unit, this));
        std::thread c(std::bind(&MemMgrAllocatorTest::unit, this));
        std::thread d(std::bind(&MemMgrAllocatorTest::unit, this));
#endif
        unit();
        a.join();
        b.join();
        c.join();
        d.join();
        memmgr_destroy();
        printf("OK\n");
    }
};

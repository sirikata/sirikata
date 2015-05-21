#include <stdlib.h>
#if __GXX_EXPERIMENTAL_CXX0X__ || __cplusplus > 199711L
#define USE_BUILTIN_ATOMICS_FOR_ALLOCATOR
#include <atomic>
#else
#include <sirikata/core/util/AtomicTypes.hpp>
#endif
#include <sirikata/core/jpeg-arhc/DecoderPlatform.hpp>
#include <sirikata/core/jpeg-arhc/BumpAllocator.hpp>
namespace Sirikata {
class BumpAllocator {
#ifdef USE_BUILTIN_ATOMICS_FOR_ALLOCATOR
    std::atomic<uint32> allocated;
    std::atomic<uint64> last_allocated;
#else
    AtomicValue<uint32> allocated;
    AtomicValue<uint64> last_allocated;
#endif
    uint64 max;
    uint8 *data_start;
    void *arena;

public:
    BumpAllocator() : allocated(0) {
        arena = data_start = NULL;
        max = 0;
    }
    size_t amount_allocated() const {
        return allocated.load();
    }
    size_t amount_free() const {
        size_t used = allocated.load();
        if (max > used) {
            return max - used;
        }
        return 0;
    }
    void *alloc(size_t amount) {
        uint32_t offset = (allocated += amount);
        if (offset > max) {
            //allocated -= amount; // FIXME <-- can this help?
            return NULL;
        }
        if (amount <= 0xffffffff) {
            uint64 last_allocated_amount = amount;
            last_allocated_amount <<= 16;
            last_allocated_amount <<= 16;
            last_allocated_amount |= offset - amount;
            last_allocated = last_allocated_amount;
        }
        return data_start + offset - amount;
    }
    void free(void *ptr) {
        uint64 la = last_allocated.load();
        uint64 new_offset = la & 0xffffffff;
        if (data_start + new_offset == ptr) {
            uint64 amount = la;
            amount >>= 16;
            amount >>= 16;
#ifdef USE_BUILTIN_ATOMICS_FOR_ALLOCATOR
            uint32 old_offset = new_offset + amount;
            allocated.compare_exchange_weak(old_offset, (uint32)new_offset);
#endif
        }
            
    }
    BumpAllocator& setup(size_t arena_size) {
        max = arena_size;
        arena = malloc(arena_size + 15);
        data_start = (uint8*)arena;
        size_t rem = ((data_start- (uint8*)0) & 0xf);
        if (rem) {
            data_start += 0x10 - rem;
        }
#ifdef USE_BUILTIN_ATOMICS_FOR_ALLOCATOR
        while (true) {
            uint32 tmp = allocated.load();
            if (allocated.compare_exchange_weak(tmp, (uint32)0)) {
                break;
            }
        }
#else
        allocated = AtomicValue<uint32>(0);
#endif
        return *this;
    }
    BumpAllocator& teardown() {
        data_start = NULL;
        free(arena);
        max = 0;
#ifdef USE_BUILTIN_ATOMICS_FOR_ALLOCATOR
        while (true) {
            uint32 tmp = allocated.load();
            if (allocated.compare_exchange_weak(tmp, (uint32)0)) {
                break;
            }
        }
#else
        allocated = AtomicValue<uint32>(0);
#endif
        return *this;
    }
};


bool BumpAllocatorTest() {
    bool ok = true;
    BumpAllocator alloc;
    alloc.setup(1024);
    void * a = alloc.alloc(512);
    ok = ok && a != NULL;
    void * b = alloc.alloc(256);
    ok = ok && b != NULL;
    void * c = alloc.alloc(256);
    ok = ok && c != NULL;
    ok = ok && alloc.amount_free() == 0;
    //fprintf(stderr, "Checking free = 0 : %d\n", ok);
    alloc.free(c);
#ifdef USE_BUILTIN_ATOMICS_FOR_ALLOCATOR
    ok = ok && alloc.amount_free() == 256;
    //fprintf(stderr, "Checking free = 256 : %d\n", ok);
    void * d = alloc.alloc(128);
    ok = ok && alloc.amount_free() == 128;
    //fprintf(stderr, "Checking free = 128 : %d\n", ok);
#endif
    void * e = alloc.alloc(256);
    ok = ok && e == NULL;
    //fprintf(stderr, "Checking too much allocated = NULL: %d", ok);
    alloc.teardown();
    alloc.setup(128);
    a = alloc.alloc(127);
    b = alloc.alloc(2);
    c = alloc.alloc(1);
    ok = ok && b == NULL;
    //fprintf(stderr, "Checking new too much allocated = NULL : %d\n", ok);
    ok = ok && alloc.amount_free() == 0;
    //fprintf(stderr, "Checking free = 0 : %d\n", ok);
    return ok;
}
void *BumpAllocatorMalloc(void *opaque, size_t nmemb, size_t size) {
    return ((BumpAllocator*)opaque)->alloc(nmemb * size);
}
void BumpAllocatorFree (void *opaque, void *ptr) {
    ((BumpAllocator*)opaque)->free(ptr);
}
void * BumpAllocatorInit(size_t prealloc_size) {
    return &(new BumpAllocator())->setup(prealloc_size);
}
void BumpAllocatorDestroy(void *opaque) {
    delete &((BumpAllocator*)opaque)->teardown();
}

}

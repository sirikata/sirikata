#include <stdlib.h>
#include <assert.h>
#include <string.h>
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
    std::atomic<size_t> allocated;
#else
    AtomicValue<size_t> allocated;
#endif
    uint64 max;
    uint8 *data_start;
    void *arena;
    uint8 alignment;
public:
    BumpAllocator() : allocated(0) {
        arena = data_start = NULL;
        max = 0;
        alignment = 1;
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
    size_t msize(void *ptr) {
        size_t amount_allocated = 0;
        if (ptr) {
            memcpy(&amount_allocated, (char*)ptr - sizeof(size_t), sizeof(size_t));
        }
        return amount_allocated;
    }
    void *alloc(size_t amount) {
        return realloc(NULL, amount, &amount, true);
    }
    void *realloc(void * ptr, size_t amount, size_t *ret_size, bool movable) {
        assert(amount > 0 && "Free should have been called if 0 size");
        size_t ptr_actual_size = msize(ptr);
        if (ptr_actual_size >= amount) {
            *ret_size = ptr_actual_size;
            return ptr;
        }
        if (ptr && !movable) {
            return NULL;
        }
        if (ptr) {
            this->free(ptr);
        }
        size_t amount_requested = amount + sizeof(size_t); // amount + size of amount
        if ((amount_requested & (alignment - 1)) != 0) {
            amount_requested += alignment - (amount_requested & (alignment - 1));
        }        
        size_t offset = (allocated += amount_requested);
        if (offset > max) {
            //allocated -= amount; // FIXME <-- can this help?
            return NULL;
        }
        size_t amount_returned = amount_requested - sizeof(size_t);
        if (ret_size) {
            *ret_size = amount_returned;
        }
        memcpy(data_start + offset - amount_requested, &amount_returned, sizeof(size_t));
        uint8* retval = data_start + offset - amount_returned;
        assert(((size_t)retval & (alignment - 1)) == 0);
        return retval;
    }
    void free(void *ptr) {
        if (ptr == NULL) {
            return;
        }
        
        size_t amount_requested = msize(ptr);
        size_t amount_with_header = amount_requested + sizeof(size_t);
        size_t new_offset = (size_t)((uint8*)ptr - data_start) - sizeof(size_t);
#ifdef USE_BUILTIN_ATOMICS_FOR_ALLOCATOR
        size_t original_check = new_offset + amount_with_header;
        allocated.compare_exchange_weak(original_check, new_offset);
#endif
    }

    BumpAllocator& setup(size_t arena_size, uint8_t alignment) {
        this->alignment = alignment;
        max = arena_size;
        arena = malloc(arena_size + alignment - 1);
        data_start = (uint8*)arena;
        size_t rem = (((data_start - (uint8*)0) + sizeof(size_t)) & (alignment - 1));
        if (rem) {
            data_start += alignment - rem;
        }
#ifdef USE_BUILTIN_ATOMICS_FOR_ALLOCATOR
        while (true) {
            size_t tmp = allocated.load();
            if (allocated.compare_exchange_weak(tmp, (size_t)0)) {
                break;
            }
        }
#else
        allocated = AtomicValue<size_t>(0);
#endif
        return *this;
    }
    BumpAllocator& teardown() {
        data_start = NULL;
        free(arena);
        max = 0;
#ifdef USE_BUILTIN_ATOMICS_FOR_ALLOCATOR
        while (true) {
            size_t tmp = allocated.load();
            if (allocated.compare_exchange_weak(tmp, (size_t)0)) {
                break;
            }
        }
#else
        allocated = AtomicValue<size_t>(0);
#endif
        return *this;
    }
};


bool BumpAllocatorTest() {
    bool ok = true;
    BumpAllocator alloc;
    {
        alloc.setup(1024 + sizeof(size_t) * 4, 1);
        void * a = alloc.alloc(512);
        ok = ok && a != NULL;
        void * b = alloc.alloc(256);
        ok = ok && b != NULL;
        void * c = alloc.alloc(256);
        ok = ok && c != NULL;
        ok = ok && alloc.amount_free() == sizeof(size_t);
        //fprintf(stderr, "Checking free = 0 : %d\n", ok);
        alloc.free(c);
#ifdef USE_BUILTIN_ATOMICS_FOR_ALLOCATOR
        ok = ok && alloc.amount_free() == 256 + sizeof(size_t) * 2;
        //fprintf(stderr, "Checking free = 256 : %d\n", ok);
        void * d = alloc.alloc(128);
        ok = ok && alloc.amount_free() == 128 + sizeof(size_t);
        //fprintf(stderr, "Checking free = 128 : %d\n", ok);
#endif
        void * e = alloc.alloc(256);
        ok = ok && e == NULL;
        //fprintf(stderr, "Checking too much allocated = NULL: %d", ok);
        alloc.teardown();
        alloc.setup(128 + sizeof(size_t) * 2, 1);
        a = alloc.alloc(127);
        b = alloc.alloc(2);
        c = alloc.alloc(1);
        ok = ok && b == NULL;
        //fprintf(stderr, "Checking new too much allocated = NULL : %d\n", ok);
        ok = ok && alloc.amount_free() == 0;
    }
    alloc.teardown();
    {
        alloc.setup(1024 + sizeof(size_t) * 8, 16);
        void * a = alloc.alloc(512);
        ok = ok && a != NULL;
        void * b = alloc.alloc(256);
        ok = ok && b != NULL;
        void * c = alloc.alloc(256);
        ok = ok && c != NULL;
        ok = ok && alloc.amount_free() == sizeof(size_t) * 2;
        //fprintf(stderr, "Checking free = 0 : %d\n", ok);
        alloc.free(c);
#ifdef USE_BUILTIN_ATOMICS_FOR_ALLOCATOR
        ok = ok && alloc.amount_free() == 256 + sizeof(size_t) * 4;
        //fprintf(stderr, "Checking free = 256 : %d\n", ok);
        void * d = alloc.alloc(128);
        ok = ok && alloc.amount_free() == 128 + sizeof(size_t) * 2;
        //fprintf(stderr, "Checking free = 128 : %d\n", ok);
#endif
        void * e = alloc.alloc(256);
        ok = ok && e == NULL;
        //fprintf(stderr, "Checking too much allocated = NULL: %d", ok);
        alloc.teardown();
    }
    //fprintf(stderr, "Checking free = 0 : %d\n", ok);
    return ok;
}
void *BumpAllocatorMalloc(void *opaque, size_t nmemb, size_t size) {
    return ((BumpAllocator*)opaque)->realloc(NULL, nmemb * size, &size, true);
}
size_t BumpAllocatorMsize(void * ptr, void *opaque) {
    return ((BumpAllocator*)opaque)->msize(ptr);
}
void *BumpAllocatorRealloc(void * ptr, size_t size, size_t *ret_size, unsigned int movable, void *opaque) {
    if (size == 0) {
        ((BumpAllocator*)opaque)->free(ptr);
        return NULL;
    }
    return ((BumpAllocator*)opaque)->realloc(ptr, size, ret_size, movable);
}

void BumpAllocatorFree (void *opaque, void *ptr) {
    ((BumpAllocator*)opaque)->free(ptr);
}
void * BumpAllocatorInit(size_t prealloc_size, unsigned char alignment) {
    return &(new BumpAllocator())->setup(prealloc_size, alignment);
}
void BumpAllocatorDestroy(void *opaque) {
    delete &((BumpAllocator*)opaque)->teardown();
}

}

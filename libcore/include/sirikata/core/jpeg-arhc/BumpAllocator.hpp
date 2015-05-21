namespace Sirikata {
SIRIKATA_FUNCTION_EXPORT void *BumpAllocatorMalloc(void *opaque, size_t nmemb, size_t size);
SIRIKATA_FUNCTION_EXPORT void BumpAllocatorFree (void *opaque, void *ptr);
SIRIKATA_FUNCTION_EXPORT void * BumpAllocatorInit(size_t prealloc_size);
SIRIKATA_FUNCTION_EXPORT void BumpAllocatorDestroy(void *opaque);
SIRIKATA_FUNCTION_EXPORT bool BumpAllocatorTest(); // runs standard tests on the BumpAllocator
}


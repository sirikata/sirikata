/*  Sirikata Jpeg Texture Transfer -- Texture Transfer management system
 *  main.cpp
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
namespace Sirikata {
SIRIKATA_FUNCTION_EXPORT void *BumpAllocatorMalloc(void *opaque, size_t nmemb, size_t size);
SIRIKATA_FUNCTION_EXPORT void BumpAllocatorFree (void *opaque, void *ptr);
SIRIKATA_FUNCTION_EXPORT void * BumpAllocatorInit(size_t prealloc_size, unsigned char alignment);
SIRIKATA_FUNCTION_EXPORT void BumpAllocatorDestroy(void *opaque);
SIRIKATA_FUNCTION_EXPORT void* BumpAllocatorRealloc(void * ptr, size_t size, size_t *actualSize, unsigned int movable, void *opaque);
SIRIKATA_FUNCTION_EXPORT size_t BumpAllocatorMsize(void * ptr, void *opaque);

SIRIKATA_FUNCTION_EXPORT bool BumpAllocatorTest(); // runs standard tests on the BumpAllocator
}


/*  Sirikata Utilities -- Dynamic Library Loading
 *  DynamicLibrary.cpp
 *
 *  Copyright (c) 2009, Ewen Cheslack-Postava
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
 *  * Neither the name of Sirikata nor the names of its contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
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

#include "util/DynamicLibrary.hpp"

#if SIRIKATA_PLATFORM == PLATFORM_WINDOWS
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#elif SIRIKATA_PLATFORM == PLATFORM_MAC || SIRIKATA_PLATFORM == PLATFORM_LINUX
#  include <dlfcn.h>
#endif

namespace Sirikata {

DynamicLibrary::DynamicLibrary(const String& path)
 : mPath(path),
   mHandle(NULL)
{
}

DynamicLibrary::~DynamicLibrary() {
    if (mHandle != NULL)
        unload();
}

bool DynamicLibrary::load() {
    if (mHandle != NULL)
        return true;

#if SIRIKATA_PLATFORM == PLATFORM_WINDOWS
    mHandle = LoadLibrary(mPath.c_str());
#elif SIRIKATA_PLATFORM == PLATFORM_MAC || SIRIKATA_PLATFORM == PLATFORM_LINUX
    mHandle = dlopen(mPath.c_str(), RTLD_LAZY | RTLD_GLOBAL);
#endif

    return (mHandle != NULL);
}

bool DynamicLibrary::unload() {
    if (mHandle == NULL)
        return true;

#if SIRIKATA_PLATFORM == PLATFORM_WINDOWS
    bool success = !FreeLibrary(mHandle);
#elif SIRIKATA_PLATFORM == PLATFORM_MAC || SIRIKATA_PLATFORM == PLATFORM_LINUX
    bool success = dlclose(mHandle);
#endif

    return success;
}

void* DynamicLibrary::symbol(const String& name) const {
#if SIRIKATA_PLATFORM == PLATFORM_WINDOWS
    return (void*)GetProcAddress(mHandle, name.c_str());
#elif SIRIKATA_PLATFORM == PLATFORM_MAC || SIRIKATA_PLATFORM == PLATFORM_LINUX
    return (void*)dlsym(mHandle, name.c_str());
#endif
}

String DynamicLibrary::prefix() {
#if SIRIKATA_PLATFORM == PLATFORM_WINDOWS
    return "";
#elif SIRIKATA_PLATFORM == PLATFORM_MAC
    return "lib";
#elif SIRIKATA_PLATFORM == PLATFORM_LINUX
    return "lib";
#endif
}

String DynamicLibrary::extension() {
#if SIRIKATA_PLATFORM == PLATFORM_WINDOWS
    return ".dll";
#elif SIRIKATA_PLATFORM == PLATFORM_MAC
    return ".dylib";
#elif SIRIKATA_PLATFORM == PLATFORM_LINUX
    return ".so";
#endif
}

String DynamicLibrary::filename(const String& name) {
    return ( prefix() + name + extension() );
}

String DynamicLibrary::filename(const String& path, const String& name) {
#if SIRIKATA_PLATFORM == PLATFORM_WINDOWS
    const char file_separator = '\\';
#elif SIRIKATA_PLATFORM == PLATFORM_MAC || SIRIKATA_PLATFORM == PLATFORM_LINUX
    const char file_separator = '/';
#endif

    String result = path;
    if (result.size() > 0 && result[result.size()-1] != file_separator)
        result += file_separator;
    result += filename(name);

    return result;
}

} // namespace Sirikata

/*  Sirikata Utilities -- Dynamic Library Loading
 *  DynamicLibrary.hpp
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

#ifndef _SIRIKATA_DYNAMIC_LIBRARY_HPP_
#define _SIRIKATA_DYNAMIC_LIBRARY_HPP_

#if SIRIKATA_PLATFORM == PLATFORM_WINDOWS
struct HINSTANCE__;
#  define DL_HANDLE struct HINSTANCE__*
#elif SIRIKATA_PLATFORM == PLATFORM_MAC || SIRIKATA_PLATFORM == PLATFORM_LINUX
#  define DL_HANDLE void*
#endif

namespace Sirikata {

/** DynamicLibrary represents a dynamically loadable module. This only handles
 *  loading, unloading, and symbol lookup.  This presumes nothing about the
 *  interface provided by the module.
 */
class DynamicLibrary {
public:
    DynamicLibrary(const String& path);
    ~DynamicLibrary();

    bool load();
    bool unload();

    void* symbol(const String& name) const;

    /** Get the standard platform-specific library filename prefix. */
    static String prefix();
    /** Get the standard platform-specific library filename extension. */
    static String extension();
    /** Construct a platform dependent filename for a dynamic library based
     *  on a path and library name.
     */
    static String filename(const String& path, const String& name);
    /** Construct a platform dependent filename for a dynamic library.  The
     *  result will be relative to the current path.
     */
    static String filename(const String& name);
private:
    /** Returns true if the library's filename matches the pattern required
     *  for shared libraries on this platform.
     */
    bool isValidLibraryFilename() const;

    String mPath;
    DL_HANDLE mHandle;
}; // class DynamicLibrary

} // namespace Sirikata

#endif //_SIRIKATA_DYNAMIC_LIBRARY_HPP_

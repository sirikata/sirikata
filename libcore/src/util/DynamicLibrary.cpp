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

#include <sirikata/core/util/Standard.hh>
#include <sirikata/core/util/DynamicLibrary.hpp>

#include <boost/filesystem.hpp>

#if SIRIKATA_PLATFORM == PLATFORM_WINDOWS
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#elif SIRIKATA_PLATFORM == PLATFORM_MAC || SIRIKATA_PLATFORM == PLATFORM_LINUX
#  include <dlfcn.h>
#endif

#if SIRIKATA_PLATFORM == PLATFORM_MAC
#include <mach-o/dyld.h>
#endif

namespace Sirikata {

static std::vector<String> DL_search_paths;

String DynamicLibrary::GetExecutablePath() {
#if SIRIKATA_PLATFORM == PLATFORM_MAC
    // Executable path can have relative references ("..") depending on
    // how the app was launched.
    uint32_t executable_length = 0;
    _NSGetExecutablePath(NULL, &executable_length);
    std::string executable_path(executable_length, '\0');
    char* executable_path_c = (char*)executable_path.c_str();
    int rv = _NSGetExecutablePath(executable_path_c, &executable_length);
    assert(rv == 0);
    if ((rv != 0) || (executable_path.empty()))
        return "";
    boost::filesystem::path exe_path(executable_path);
    return exe_path.parent_path().string();
#elif SIRIKATA_PLATFORM == PLATFORM_LINUX
    // boost::filesystem can't chase symlinks, do it manually
    const char* selfExe = "/proc/self/exe";
#define SIRIKATA_DL_PATH_MAX 1024
    char bin_dir[SIRIKATA_DL_PATH_MAX + 1];
    int bin_dir_size = readlink(selfExe, bin_dir, SIRIKATA_DL_PATH_MAX);
    if (bin_dir_size < 0 || bin_dir_size > SIRIKATA_DL_PATH_MAX) {
        SILOG(core,fatal,"Couldn't read self symlink to setup dynamic loading paths.");
        return "";
    }
    bin_dir[bin_dir_size] = 0;
    boost::filesystem::path exe_path(String(bin_dir, bin_dir_size));
    return exe_path.parent_path().string();
#undef SIRIKATA_DL_PATH_MAX
#else
    return "";
#endif
}

void DynamicLibrary::Initialize() {
    using namespace boost::filesystem;
#if SIRIKATA_PLATFORM == PLATFORM_MAC
    // On mac, we might be in a .app, specifically at .app/Contents. To load the
    // libs we need, we add .app/Contents/MacOS to the LD_LIBRARY_PATH
    path to_macos_dir = boost::filesystem::complete(path(".")) / path("MacOS");
    if (exists(to_macos_dir) && is_directory(to_macos_dir))
        AddLoadPath(to_macos_dir.string());
#endif

    // On all platforms, allow plugins to load from within the same
    // directory as the executable
    String exe_path = GetExecutablePath();
    if (!exe_path.empty())
        AddLoadPath(exe_path);
}

void DynamicLibrary::AddLoadPath(const String& path) {
#if SIRIKATA_PLATFORM == PLATFORM_LINUX
#define LD_LIBRARY_PATH_STR "LD_LIBRARY_PATH"
    {
        String oldLdLibraryPath = getenv(LD_LIBRARY_PATH_STR)?getenv(LD_LIBRARY_PATH_STR):"";
        String ldLibraryPath = path;
        if (!oldLdLibraryPath.empty())
            ldLibraryPath = ldLibraryPath + ":" + oldLdLibraryPath;
        setenv(LD_LIBRARY_PATH_STR,ldLibraryPath.c_str(),1);
    }
#endif
    // OSX and Ubuntu 10.04 don't seem to like setting DYLD_LIBRARY_PATH dynamically. Instead,
    // we add to the search paths and handle the search ourselves.
    DL_search_paths.push_back(path);
}

DynamicLibrary::DynamicLibrary(const String& path)
 : mPath(path),
   mHandle(NULL)
{
}
static bool unloadLibrary(DL_HANDLE lib) {
#if SIRIKATA_PLATFORM == PLATFORM_WINDOWS
    bool success = !FreeLibrary(lib);
#elif SIRIKATA_PLATFORM == PLATFORM_MAC || SIRIKATA_PLATFORM == PLATFORM_LINUX
    bool success = dlclose(lib)==0;
#endif
    return success;
}
void DynamicLibrary::gc(DL_HANDLE handle) {
    static std::vector<DL_HANDLE> *sHandles=new std::vector<DL_HANDLE>;
    if (handle) {
        if (sHandles==NULL)
            sHandles=new std::vector<DL_HANDLE>;
        sHandles->push_back(handle);
    }else if(sHandles){
        for (std::vector<DL_HANDLE>::iterator i=sHandles->begin();i!=sHandles->end();++i) {
            unloadLibrary(*i);
        }
        delete sHandles;
        sHandles=NULL;
    }
}
DynamicLibrary::~DynamicLibrary() {
    if (mHandle != NULL)
        gc(mHandle);
}

bool DynamicLibrary::load() {
    if (mHandle != NULL)
        return true;

    if (!isValidLibraryFilename())
        return false;

#if SIRIKATA_PLATFORM == PLATFORM_WINDOWS
    mHandle = LoadLibrary(mPath.c_str());
    if (mHandle == NULL) {
        // Try any registered search paths
        for(uint32 i = 0; mHandle == NULL && i < DL_search_paths.size(); i++) {
            std::string str =(boost::filesystem::path(DL_search_paths[i]) / mPath).string();
            mHandle = LoadLibrary(str.c_str());
        }
    }
    if (mHandle == NULL) {
        DWORD errnum = GetLastError();
        SILOG(plugin,error,"Failed to open library "<<mPath<<": "<<errnum);
    }
#elif SIRIKATA_PLATFORM == PLATFORM_MAC || SIRIKATA_PLATFORM == PLATFORM_LINUX
    mHandle = dlopen(mPath.c_str(), RTLD_LAZY | RTLD_GLOBAL);
    if (mHandle == NULL) {
        // Try any registered search paths
        for(uint32 i = 0; mHandle == NULL && i < DL_search_paths.size(); i++) {
            std::string str = (boost::filesystem::path(DL_search_paths[i]) / mPath).string();
            mHandle = dlopen(
                str.c_str(),
                RTLD_LAZY | RTLD_GLOBAL
            );
        }
    }
    if (mHandle == NULL) {
        const char *errorstr = dlerror();
        SILOG(plugin,error,"Failed to open library "<<mPath<<": "<<errorstr);
    }
#endif

    return (mHandle != NULL);
}

bool DynamicLibrary::unload() {
    if (mHandle == NULL)
        return true;
    bool success=unloadLibrary(mHandle);
    mHandle=NULL;
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

String DynamicLibrary::postfix() {
#if SIRIKATA_DEBUG
    return "_d";
#else
    return "";
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
    return ( prefix() + name + postfix() + extension() );
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

bool DynamicLibrary::isValidLibraryFilename() const {
    using namespace boost::filesystem;

    String pref = prefix();
    String ext = extension();

    path bfs_path = mPath;
    // FIXME this should be bfs_path.leaf() in 1.35, bfs_path.filename() in
    // 1.37.  Instead, just do this manually.
    String libfilename = bfs_path.empty() ? String() : *--bfs_path.end();

    if (libfilename.substr(0, pref.size()) != pref)
        return false;

    if (libfilename.size() < ext.size() ||
        libfilename.substr(libfilename.size() - ext.size()) != ext )
        return false;

    return true;
}

} // namespace Sirikata

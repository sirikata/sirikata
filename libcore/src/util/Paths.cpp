// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include <sirikata/core/util/Standard.hh>
#include <sirikata/core/util/Paths.hpp>
#include <boost/filesystem.hpp>

#if SIRIKATA_PLATFORM == PLATFORM_MAC
#include <mach-o/dyld.h>
#endif

#ifndef MAX_PATH
#define MAX_PATH 1024
#endif

namespace Sirikata {
namespace Path {

String Get(Key key) {
    switch(key) {

      case FILE_EXE:
          {
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
              return executable_path;
#elif SIRIKATA_PLATFORM == PLATFORM_LINUX
              // boost::filesystem can't chase symlinks, do it manually
              const char* selfExe = "/proc/self/exe";

              char bin_dir[MAX_PATH + 1];
              int bin_dir_size = readlink(selfExe, bin_dir, MAX_PATH);
              if (bin_dir_size < 0 || bin_dir_size > MAX_PATH) {
                  SILOG(core,fatal,"Couldn't read self symlink to setup dynamic loading paths.");
                  return "";
              }
              bin_dir[bin_dir_size] = 0;
              return String(bin_dir, bin_dir_size);
#elif SIRIKATA_PLATFORM == PLATFORM_WINDOWS
              char system_buffer[MAX_PATH];
              system_buffer[0] = 0;
              GetModuleFileName(NULL, system_buffer, MAX_PATH);
              return String(system_buffer);
#else
              return "";
#endif
          }
          break;

      case DIR_EXE:
          {
              String exe_file = Get(FILE_EXE);
              if (exe_file.empty()) return "";
              boost::filesystem::path exe_file_path(exe_file);
              return exe_file_path.parent_path().string();
          }
          break;


      case DIR_CURRENT:
          {
#if SIRIKATA_PLATFORM == PLATFORM_MAC || SIRIKATA_PLATFORM == PLATFORM_LINUX
              char system_buffer[MAX_PATH] = "";
              if (!getcwd(system_buffer, sizeof(system_buffer))) {
                  return "";
              }

              return String(system_buffer);
#elif SIRIKATA_PLATFORM == PLATFORM_WINDOWS
              char system_buffer[MAX_PATH];
              system_buffer[0] = 0;
              DWORD len = ::GetCurrentDirectory(MAX_PATH, system_buffer);
              if (len == 0 || len > MAX_PATH)
                  return "";
              return String(system_buffer);
#else
              return ".";
#endif
          }
          break;

      default:
        return "";
    }
}

} // namespace Path
} // namespace Sirikata

// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include <sirikata/core/util/Standard.hh>
#include <sirikata/core/util/Paths.hpp>
#include <boost/filesystem.hpp>

#if SIRIKATA_PLATFORM == SIRIKATA_PLATFORM_MAC
#include <mach-o/dyld.h>
#endif

#if SIRIKATA_PLATFORM == SIRIKATA_PLATFORM_MAC || SIRIKATA_PLATFORM == SIRIKATA_PLATFORM_LINUX
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#endif

#if SIRIKATA_PLATFORM == SIRIKATA_PLATFORM_WINDOWS
#include <shlobj.h>
#endif

#ifndef MAX_PATH
#define MAX_PATH 1024
#endif

#include <sirikata/core/util/Random.hpp>

namespace Sirikata {
namespace Path {

namespace Placeholders {
const String DIR_EXE("<bindir>");
const String DIR_EXE_BUNDLE("<bundledir>");
const String DIR_CURRENT("<currentdir>");
const String DIR_USER("<userdir>");
const String DIR_USER_HIDDEN("<hiddenuserdir>");
const String DIR_TEMP("<temp>");
const String DIR_SYSTEM_CONFIG("<sysconfig>");

String RESOURCE(const String& intree, const String& resource) {
    return "<resource:" + intree + ":" + resource + ">";
}

typedef std::pair<String, Key> PlaceholderPair;
const PlaceholderPair All[] = {
    PlaceholderPair(Sirikata::Path::Placeholders::DIR_EXE, Sirikata::Path::DIR_EXE),
    PlaceholderPair(Sirikata::Path::Placeholders::DIR_EXE_BUNDLE, Sirikata::Path::DIR_EXE_BUNDLE),
    PlaceholderPair(Sirikata::Path::Placeholders::DIR_CURRENT, Sirikata::Path::DIR_CURRENT),
    PlaceholderPair(Sirikata::Path::Placeholders::DIR_USER, Sirikata::Path::DIR_USER),
    PlaceholderPair(Sirikata::Path::Placeholders::DIR_USER_HIDDEN, Sirikata::Path::DIR_USER_HIDDEN),
    PlaceholderPair(Sirikata::Path::Placeholders::DIR_TEMP, Sirikata::Path::DIR_TEMP),
    PlaceholderPair(Sirikata::Path::Placeholders::DIR_SYSTEM_CONFIG, Sirikata::Path::DIR_SYSTEM_CONFIG),
};
} // namespace Placeholders


namespace {

// Canonicalize a path, i.e. clean out unnecessary components such as '.'s. If
// aggressive is true, also removes '..'s. Since that operation isn't always
// safe, it's not the default.
boost::filesystem::path canonicalize(const boost::filesystem::path& orig, bool aggressive=false) {
    using namespace boost::filesystem;
    path result;

    for(path::iterator it = orig.begin(); it != orig.end(); it++) {
        if (*it == ".") {
            // Do nothing to get rid of it
        }
        else if (*it == ".." && aggressive) {
            // If available, pop off the previous thing
            result = result.parent_path();
        }
        else {
            result /= *it;
        }
    }

    return result;
}
String canonicalize(const String& p, bool aggressive=false) {
    return canonicalize( boost::filesystem::path(p), aggressive).string();
}
}

String Get(Key key) {
    switch(key) {

      case FILE_EXE:
          {
#if SIRIKATA_PLATFORM == SIRIKATA_PLATFORM_MAC
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
              // _NSGetExecutablePath will return whatever gets execed, so if
              // the command line is ./foo, you'll get the '.'. We use the
              // aggressive mode here to handle '..' parts that could interfere
              // with finding other paths that start from FILE_EXE.
              return canonicalize(executable_path, true);
#elif SIRIKATA_PLATFORM == SIRIKATA_PLATFORM_LINUX
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
#elif SIRIKATA_PLATFORM == SIRIKATA_PLATFORM_WINDOWS
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

      case DIR_EXE_BUNDLE:
          {
#if SIRIKATA_PLATFORM == SIRIKATA_PLATFORM_WINDOWS || SIRIKATA_PLATFORM == SIRIKATA_PLATFORM_LINUX
              // Windows and Linux don't have bundles
              return Get(DIR_EXE);
#elif SIRIKATA_PLATFORM == SIRIKATA_PLATFORM_MAC
              // On mac we need to detect that we're in a .app. We assume this
              // only applies if the binaries are in the standard location,
              // i.e. foo.app/Contents/MacOS/bar_binary
              String exe_dir = Get(DIR_EXE);
              boost::filesystem::path exe_dir_path(exe_dir);
              // Work our way back up verifying the path names, finally
              // returning if we actually find the .app.
              if (exe_dir_path.has_filename() && exe_dir_path.filename() == "MacOS") {
                  exe_dir_path = exe_dir_path.parent_path();
                  if (exe_dir_path.has_filename() && exe_dir_path.filename() == "Contents") {
                      exe_dir_path = exe_dir_path.parent_path();
                      if (exe_dir_path.has_filename()) {
                          String app_dir_name = exe_dir_path.filename();
                          if (app_dir_name.substr(app_dir_name.size()-4, 4) == ".app")
                              return exe_dir_path.parent_path().string();
                      }
                  }
              }
              // Otherwise dump the original
              return exe_dir;
#endif
          }
          break;

      case DIR_CURRENT:
          {
#if SIRIKATA_PLATFORM == SIRIKATA_PLATFORM_MAC || SIRIKATA_PLATFORM == SIRIKATA_PLATFORM_LINUX
              char system_buffer[MAX_PATH] = "";
              if (!getcwd(system_buffer, sizeof(system_buffer))) {
                  return "";
              }

              return String(system_buffer);
#elif SIRIKATA_PLATFORM == SIRIKATA_PLATFORM_WINDOWS
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

      case DIR_USER:
          {
#if SIRIKATA_PLATFORM == SIRIKATA_PLATFORM_LINUX || SIRIKATA_PLATFORM == SIRIKATA_PLATFORM_MAC
              uid_t uid = getuid();
              passwd* pw = getpwuid(uid);
              if (pw != NULL && pw->pw_dir != NULL) {
                  boost::filesystem::path homedir(pw->pw_dir);
                  if (boost::filesystem::exists(homedir) && boost::filesystem::is_directory(homedir))
                      return homedir.string();
              }
#elif SIRIKATA_PLATFORM == SIRIKATA_PLATFORM_WINDOWS
              char system_buffer[MAX_PATH];
              system_buffer[0] = 0;
              if (FAILED(SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, system_buffer)))
                  return "";
              std::string appdata_str(system_buffer);
              boost::filesystem::path user_appdata(appdata_str);
              user_appdata /= "Sirikata";
              if (!boost::filesystem::exists(user_appdata))
                  boost::filesystem::create_directory(user_appdata);
              if (boost::filesystem::exists(user_appdata) && boost::filesystem::is_directory(user_appdata))
                  return user_appdata.string();
#endif
              // Last resort (and default for unknown platform) is to try to use
              // the current directory
              return ".";
          }
          break;

      case DIR_USER_HIDDEN:
          {
#if SIRIKATA_PLATFORM == SIRIKATA_PLATFORM_WINDOWS
              // On windows there's no difference from the user-specific data directory since that's already hidden.
              return Get(DIR_USER);
#else
              // We just compute this as an offset from the user directory
              boost::filesystem::path user_dir(Get(DIR_USER));
              user_dir /= ".sirikata";
              if (!boost::filesystem::exists(user_dir))
                  boost::filesystem::create_directory(user_dir);
              if (boost::filesystem::exists(user_dir) && boost::filesystem::is_directory(user_dir))
                  return user_dir.string();
#endif
              return ".";
          }

      case DIR_TEMP:
          {
#if SIRIKATA_PLATFORM == SIRIKATA_PLATFORM_LINUX || SIRIKATA_PLATFORM == SIRIKATA_PLATFORM_MAC
              // On Mac and Linux we try to work under tmp using our own directory
              boost::filesystem::path tmp_path("/tmp");
              if (boost::filesystem::exists(tmp_path) && boost::filesystem::is_directory(tmp_path)) {
                  tmp_path /= "sirikata";
                  // If it doesn't exist, try creating it
                  if (!boost::filesystem::exists(tmp_path))
                      boost::filesystem::create_directory(tmp_path);
                  if (boost::filesystem::exists(tmp_path) && boost::filesystem::is_directory(tmp_path))
                      return tmp_path.string();
              }
#elif SIRIKATA_PLATFORM == SIRIKATA_PLATFORM_WINDOWS
              // Windows doesn't seem to suggest a good location for this, so we
              // put it under the app data directory in its own temp directory
              boost::filesystem::path sirikata_temp_dir =
                  boost::filesystem::path(Get(DIR_USER_HIDDEN)) / "temp";
              if (!boost::filesystem::exists(sirikata_temp_dir))
                  boost::filesystem::create_directory(sirikata_temp_dir);
              if (boost::filesystem::exists(sirikata_temp_dir) && boost::filesystem::is_directory(sirikata_temp_dir))
                  return sirikata_temp_dir.string();
#endif
              // Last resort (and default for unknown platform) is to try to use
              // the current directory
              return ".";
          }
          break;

      case DIR_SYSTEM_CONFIG:
          {
#if SIRIKATA_PLATFORM == SIRIKATA_PLATFORM_LINUX || SIRIKATA_PLATFORM == SIRIKATA_PLATFORM_MAC
              // This is sirikata specific, so we're looking for more
              // than just /etc.
              if (boost::filesystem::exists("/etc") && boost::filesystem::is_directory("/etc") &&
                  boost::filesystem::exists("/etc/sirikata") && boost::filesystem::is_directory("/etc/sirikata"))
                  return "/etc/sirikata";
              return "";
#else
              // Other platforms don't have an equivalent?
              return "";
#endif
          }
          break;

      case RESOURCE:
          {
              SILOG(core,fatal,"Can't request RESOURCE without specifiying an in-tree path and path to resource.");
              assert(key != RESOURCE);
              return "";
          }
          break;

      default:
        return "";
    }
}

String Get(Key key, const String& relative_path) {
    boost::filesystem::path rel_path(relative_path);
    // If they actually gave us a relative path, just hand it back
    if (rel_path.is_complete()) return relative_path;

    boost::filesystem::path base_path(Get(key));
    return (base_path / rel_path).string();
}

String Get(Key key, const String& relative_path, const String& alternate_base) {
    if (key != RESOURCE) {
        SILOG(core,fatal,"Get with alternate base paths is only supported for RESOURCE");
        assert(key == RESOURCE);
        return "";
    }

    // We can always find the resource directory as an offset from the
    // binary's directory.
    boost::filesystem::path exe_dir(Get(DIR_EXE_BUNDLE));
    // From there, we have a couple of possibilities. In the installed
    // version we'll have, e.g., /usr/bin or /usr/lib/sirikata and
    // /usr/share/sirikata
    {
        // Try for /usr/bin
        boost::filesystem::path share_dir_resource = exe_dir.parent_path() / "share" / "sirikata" / relative_path;
        if (boost::filesystem::exists(share_dir_resource))
            return share_dir_resource.string();
    }
    {
        // Try for /usr/lib/sirikata
        boost::filesystem::path share_dir_resource = exe_dir.parent_path().parent_path() / "share" / "sirikata" / relative_path;
        if (boost::filesystem::exists(share_dir_resource))
            return share_dir_resource.string();
    }
    // Otherwise we need to try the alternate base path within the
    // tree.
    {
#if SIRIKATA_PLATFORM == SIRIKATA_PLATFORM_WINDOWS
        // On windows we need to deal with the fact that we have Debug/
        // and RelWithDebInfo/ directories under build/cmake/.
        boost::filesystem::path source_base = exe_dir.parent_path().parent_path().parent_path();
#else
        // On other platforms, we just have the normal offset in build/cmake/.
        boost::filesystem::path source_base = exe_dir.parent_path().parent_path();
#endif
        boost::filesystem::path intree_resource = source_base / alternate_base / relative_path;
        if (boost::filesystem::exists(intree_resource))
            return intree_resource.string();
    }

    return "";
}

bool Set(Key key, const String& path) {
    switch(key) {

      case DIR_CURRENT:
          {
#if SIRIKATA_PLATFORM == SIRIKATA_PLATFORM_MAC || SIRIKATA_PLATFORM == SIRIKATA_PLATFORM_LINUX
              int ret = chdir(path.c_str());
              return !ret;
#elif SIRIKATA_PLATFORM == SIRIKATA_PLATFORM_WINDOWS
              BOOL ret = ::SetCurrentDirectory(path.c_str());
              return ret != 0;
#endif
          }
          break;
      default:
        return false;
        break;
    }
}


String SubstitutePlaceholders(const String& path) {
    String result = path;
    for(uint32 i = 0; i < sizeof(Placeholders::All) / sizeof(Placeholders::PlaceholderPair); i++) {
        std::size_t sub_pos = result.find(Placeholders::All[i].first);
        if (sub_pos != String::npos)
            result.replace(sub_pos, Placeholders::All[i].first.size(), Path::Get(Placeholders::All[i].second));
    }

    // Resource placeholders are handled differently because they
    // don't have a fixed pattern. They have the form
    // <resource:intree:resourcepath>.
    std::size_t resource_start_pos = result.find("<resource:");
    if (resource_start_pos != String::npos) {
        std::size_t resource_pos = resource_start_pos + 10; // sizeof("<resource:")
        std::size_t resource_end_pos = result.find(">", resource_pos);
        // Get the paths part of the resource string and split it by
        // the :
        String resource_paths = result.substr(resource_pos, resource_end_pos - resource_pos);
        std::size_t split_pos = resource_paths.find(":");
        String alternate_base = resource_paths.substr(0, split_pos);
        String resource_path = resource_paths.substr(split_pos + 1);

        // Finally lookup and replace the resource path
        String replace_path = Get(RESOURCE, resource_path, alternate_base);
        result.replace(resource_start_pos, resource_end_pos - resource_start_pos + 1, replace_path);
    }

    return result;
}

String GetTempFilename(const String& prefix) {
    static const char* random_chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890";
    static int32 num_random_chars = strlen(random_chars);

    String result = prefix;
    for(uint32 i = 0; i < 8; i++) {
        uint32 idx = randInt(0, num_random_chars-1);
        result += random_chars[idx];
    }

    return result;
}

} // namespace Path
} // namespace Sirikata

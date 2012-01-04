// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_LIBCORE_UTIL_PATHS_HPP_
#define _SIRIKATA_LIBCORE_UTIL_PATHS_HPP_

#include <sirikata/core/util/Platform.hpp>

namespace Sirikata {

/** Utilities for getting information about paths, such as the executable file,
 *  executable directory, current directory, etc.
 */
namespace Path {

enum Key {
    PATH_START = 0,

    // Full path to executable file
    FILE_EXE,
    // Full path to executable file's directory
    DIR_EXE,
    // Full path to executable file's bundle. On most platform's this is
    // equivalent to DIR_EXE. On OS X, it gives the directory of the .app
    // containing the binary when it is located in one.
    DIR_EXE_BUNDLE,
    // Full path to current directory
    DIR_CURRENT,
    // Full path to a user-specific directory, e.g. /home/username
    DIR_USER,
    // Full path to a hidden directory in a user-specific location,
    // e.g. /home/username/.sirikata
    DIR_USER_HIDDEN,
    // Full path to temporary directory, e.g. under /tmp
    DIR_TEMP,
    // System-wide configuration directory, e.g. /etc/sirikata. Note
    // that this is sirikata specific.
    DIR_SYSTEM_CONFIG,

    // Path to resources directory, e.g. share/. You can't request
    // this path directly since it's actually a collection of in-tree
    // resources: calling Get(RESOURCE) will fail. Instead, you
    // need to use the version of Get() where you also provide an
    // in-tree path (e.g. liboh/plugins/) and a path which is
    // identical in both the in-tree and installed versions
    // (e.g. js/scripts).
    RESOURCE,

    PATH_END
};

/** Path placeholders are strings that can be automatically substituted with
 *  standard locations, e.g. the binary path, the user's home directory,
 *  etc. They correspond to the Path::Key directories.
 */
namespace Placeholders {
extern SIRIKATA_EXPORT const String DIR_EXE;
extern SIRIKATA_EXPORT const String DIR_EXE_BUNDLE;
extern SIRIKATA_EXPORT const String DIR_CURRENT;
extern SIRIKATA_EXPORT const String DIR_USER;
extern SIRIKATA_EXPORT const String DIR_USER_HIDDEN;
extern SIRIKATA_EXPORT const String DIR_TEMP;
extern SIRIKATA_EXPORT const String DIR_SYSTEM_CONFIG;
// This one is a function because you must provide a search file or
// path within the resources directory.
SIRIKATA_FUNCTION_EXPORT String RESOURCE(const String& intree, const String& resource);
} // namespace Placeholders

SIRIKATA_FUNCTION_EXPORT String Get(Key key);
// Get a path from an offset based on key, e.g. use Get(DIR_TEMP, "foo.log") to
// get the equivalent of "/tmp/sirikata/foo.log".
SIRIKATA_FUNCTION_EXPORT String Get(Key key, const String& relative_path);
SIRIKATA_FUNCTION_EXPORT bool Set(Key key, const String& path);
// Special version which is used for searching when multiple (fixed)
// paths to a resource may work, e.g. because of differences in the
// directory layout between in-tree and installed versions. For
// example, Get(RESOURCE, "js/scripts", "liboh/plugins") could return
// /home/user/sirikata/liboh/plugins/js/scripts for the in-tree
// version and /usr/share/js/scripts for the installed version.
SIRIKATA_FUNCTION_EXPORT String Get(Key key, const String& relative_path, const String& alternate_base);

/** Substitute full values for placeholders in the given path. */
SIRIKATA_FUNCTION_EXPORT String SubstitutePlaceholders(const String& path);

} // namespace Path
} // namespace Sirikata

#endif //_SIRIKATA_LIBCORE_UTIL_PATHS_HPP_

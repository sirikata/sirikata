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

    FILE_EXE, // Full path to executable file,
    DIR_EXE, // Full path to executable file's directory
    DIR_CURRENT, // Full path to current directory

    PATH_END
};

SIRIKATA_FUNCTION_EXPORT String Get(Key key);

} // namespace Path
} // namespace Sirikata

#endif //_SIRIKATA_LIBCORE_UTIL_PATHS_HPP_

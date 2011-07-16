// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef SIRIKATA_CORE_UTIL_BASE64_HPP__
#define SIRIKATA_CORE_UTIL_BASE64_HPP__

#include <sirikata/core/util/Platform.hpp>

namespace Sirikata {
namespace Base64 {

SIRIKATA_FUNCTION_EXPORT String encode(const String& orig);
SIRIKATA_FUNCTION_EXPORT String encodeURL(const String& orig);
SIRIKATA_FUNCTION_EXPORT String decode(const String& orig);
SIRIKATA_FUNCTION_EXPORT String decodeURL(const String& orig);

} // namespace Base64
} // Sirikata

#endif // SIRIKATA_CORE_UTIL_BASE64_HPP__

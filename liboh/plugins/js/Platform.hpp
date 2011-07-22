// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_SCRIPTING_JS_PLATFORM_HPP_
#define _SIRIKATA_SCRIPTING_JS_PLATFORM_HPP_

#include <sirikata/core/util/Platform.hpp>

#ifndef SIRIKATA_SCRIPTING_JS_EXPORT
# if SIRIKATA_PLATFORM == PLATFORM_WINDOWS
#   if defined(STATIC_LINKED)
#     define SIRIKATA_SCRIPTING_JS_EXPORT
#   else
#     if defined(SIRIKATA_SCRIPTING_JS_BUILD)
#       define SIRIKATA_SCRIPTING_JS_EXPORT __declspec(dllexport)
#     else
#       define SIRIKATA_SCRIPTING_JS_EXPORT __declspec(dllimport)
#     endif
#   endif
#   define SIRIKATA_SCRIPTING_JS_PLUGIN_EXPORT __declspec(dllexport)
# else
#   if defined(__GNUC__) && __GNUC__ >= 4
#     define SIRIKATA_SCRIPTING_JS_EXPORT __attribute__ ((visibility("default")))
#     define SIRIKATA_SCRIPTING_JS_PLUGIN_EXPORT __attribute__ ((visibility("default")))
#   else
#     define SIRIKATA_SCRIPTING_JS_EXPORT
#     define SIRIKATA_SCRIPTING_JS_PLUGIN_EXPORT
#   endif
# endif
#endif

#ifndef SIRIKATA_SCRIPTING_JS_EXPORT_C
# define SIRIKATA_SCRIPTING_JS_EXPORT_C extern "C" SIRIKATA_SCRIPTING_JS_EXPORT
#endif

#ifndef SIRIKATA_SCRIPTING_JS_PLUGIN_EXPORT_C
# define SIRIKATA_SCRIPTING_JS_PLUGIN_EXPORT_C extern "C" SIRIKATA_SCRIPTING_JS_PLUGIN_EXPORT
#endif

#endif //_SIRIKATA_SCRIPTING_JS_PLATFORM_HPP_

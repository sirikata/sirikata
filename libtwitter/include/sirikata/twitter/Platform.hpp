// Copyright (c) 2013 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_TWITTER_PLATFORM_HPP_
#define _SIRIKATA_TWITTER_PLATFORM_HPP_

#include <sirikata/core/util/Platform.hpp>

#ifndef SIRIKATA_TWITTER_EXPORT
# if SIRIKATA_PLATFORM == SIRIKATA_PLATFORM_WINDOWS
#   if defined(STATIC_LINKED)
#     define SIRIKATA_TWITTER_EXPORT
#   else
#     if defined(SIRIKATA_TWITTER_BUILD)
#       define SIRIKATA_TWITTER_EXPORT __declspec(dllexport)
#     else
#       define SIRIKATA_TWITTER_EXPORT __declspec(dllimport)
#     endif
#   endif
#   define SIRIKATA_TWITTER_PLUGIN_EXPORT __declspec(dllexport)
# else
#   if defined(__GNUC__) && __GNUC__ >= 4
#     define SIRIKATA_TWITTER_EXPORT __attribute__ ((visibility("default")))
#     define SIRIKATA_TWITTER_PLUGIN_EXPORT __attribute__ ((visibility("default")))
#   else
#     define SIRIKATA_TWITTER_EXPORT
#     define SIRIKATA_TWITTER_PLUGIN_EXPORT
#   endif
# endif
#endif

#ifndef SIRIKATA_TWITTER_EXPORT_C
# define SIRIKATA_TWITTER_EXPORT_C extern "C" SIRIKATA_TWITTER_EXPORT
#endif

#ifndef SIRIKATA_TWITTER_PLUGIN_EXPORT_C
# define SIRIKATA_TWITTER_PLUGIN_EXPORT_C extern "C" SIRIKATA_TWITTER_PLUGIN_EXPORT
#endif

#endif //_SIRIKATA_TWITTER_PLATFORM_HPP_

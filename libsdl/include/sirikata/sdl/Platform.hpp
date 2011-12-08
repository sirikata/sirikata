// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_SDL_PLATFORM_HPP_
#define _SIRIKATA_SDL_PLATFORM_HPP_

#include <sirikata/core/util/Platform.hpp>

#ifndef SIRIKATA_SDL_EXPORT
# if SIRIKATA_PLATFORM == PLATFORM_WINDOWS
#   if defined(STATIC_LINKED)
#     define SIRIKATA_SDL_EXPORT
#   else
#     if defined(SIRIKATA_SDL_BUILD)
#       define SIRIKATA_SDL_EXPORT __declspec(dllexport)
#     else
#       define SIRIKATA_SDL_EXPORT __declspec(dllimport)
#     endif
#   endif
#   define SIRIKATA_SDL_PLUGIN_EXPORT __declspec(dllexport)
# else
#   if defined(__GNUC__) && __GNUC__ >= 4
#     define SIRIKATA_SDL_EXPORT __attribute__ ((visibility("default")))
#     define SIRIKATA_SDL_PLUGIN_EXPORT __attribute__ ((visibility("default")))
#   else
#     define SIRIKATA_SDL_EXPORT
#     define SIRIKATA_SDL_PLUGIN_EXPORT
#   endif
# endif
#endif

#ifndef SIRIKATA_SDL_FUNCTION_EXPORT
# if SIRIKATA_PLATFORM == PLATFORM_WINDOWS
#   if defined(STATIC_LINKED)
#     define SIRIKATA_SDL_FUNCTION_EXPORT
#   else
#     if defined(SIRIKATA_SDL_BUILD)
#       define SIRIKATA_SDL_FUNCTION_EXPORT __declspec(dllexport)
#     else
#       define SIRIKATA_SDL_FUNCTION_EXPORT __declspec(dllimport)
#     endif
#   endif
# else
#   if defined(__GNUC__) && __GNUC__ >= 4
#     define SIRIKATA_SDL_FUNCTION_EXPORT __attribute__ ((visibility("default")))
#   else
#     define SIRIKATA_SDL_FUNCTION_EXPORT
#   endif
# endif
#endif

#ifndef SIRIKATA_SDL_EXPORT_C
# define SIRIKATA_SDL_EXPORT_C extern "C" SIRIKATA_SDL_EXPORT
#endif

#ifndef SIRIKATA_SDL_PLUGIN_EXPORT_C
# define SIRIKATA_SDL_PLUGIN_EXPORT_C extern "C" SIRIKATA_SDL_PLUGIN_EXPORT
#endif

#endif //_SIRIKATA_SDL_PLATFORM_HPP_

// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_LIBSDL_SDL_HPP_
#define _SIRIKATA_LIBSDL_SDL_HPP_

#include <sirikata/sdl/Platform.hpp>

namespace Sirikata {
/** Provides wrappers around the basic SDL library for methods which would not
 *  be safe if used by different code (e.g. graphics and audio plugins) without
 *  coordination. Most functionality should just be used directly from SDL: this
 *  only covers unsafe shared operations like initialization, the event loop,
 *  and destruction.
 *
 *  Unlike SDL, you only deal with initializing and destroying subsystems. The
 *  library will handling the basic global initialization and destruction for
 *  you.
 */
namespace SDL {

namespace Subsystem {
enum Type {
    Timer,
    Audio,
    Video,
    CD,
    Joystick,
    NumSubsystems
};
} // namespace Subsystem

/** Initialize a single subsystem. Equivalent to SDL_InitSubsystem. Will invoke
 *  Initialize() if necessary. Like SDL_InitSubsystem, returns 0 on
 *  success, -1 in case of error.
 */
int SIRIKATA_SDL_FUNCTION_EXPORT InitializeSubsystem(Subsystem::Type sub);

/** Destroy a single subsystem. Equivalent to SDL_QuitSubsystem except that it
 *  only passes the request to SDL when all InitializeSubsystem calls have been
 *  matched.
 */
void SIRIKATA_SDL_FUNCTION_EXPORT QuitSubsystem(Subsystem::Type sub);

} // namespace SDL
} // namespace Sirikata

#endif //_SIRIKATA_LIBSDL_SDL_HPP_

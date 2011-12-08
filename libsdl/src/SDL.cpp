// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include <sirikata/core/util/Platform.hpp>
#include <sirikata/sdl/SDL.hpp>
#include <SDL.h>

namespace Sirikata {
namespace SDL {

namespace {

uint32 SDLSubsystem(Subsystem::Type sub) {
    switch (sub) {
      case Subsystem::Timer: return SDL_INIT_TIMER; break;
      case Subsystem::Audio: return SDL_INIT_AUDIO; break;
      case Subsystem::Video: return SDL_INIT_VIDEO; break;
      case Subsystem::CD: return SDL_INIT_CDROM; break;
      case Subsystem::Joystick: return SDL_INIT_JOYSTICK; break;
      default: return 0xFFFFFFFF; break;
    }
}

// We use an instance of this struct to make sure we setup and cleanup
// everything properly when this library is unloaded, as well as to track some
// state about requests. We track all of this in the same struct to ensure
// initialization/destruction order doesn't matter.
struct SafeInitCleanup {
    SafeInitCleanup()
     : initialized(false)
    {
        memset(subsystem_refcounts, 0, Subsystem::NumSubsystems);
    }

    ~SafeInitCleanup() {
        if (initialized) {
            SDL_Quit();
            initialized = false;
        }
    }

    int initialize(Subsystem::Type sub) {
        assert(sub < Subsystem::NumSubsystems);
        if (subsystem_refcounts[sub] == 0) {
            if (!initialized) {
                if (SDL_Init(SDL_INIT_NOPARACHUTE) < 0)
                    return -1;
                initialized = true;
            }
            int retval = SDL_InitSubSystem(SDLSubsystem(sub));
            // Only increase refcount upon successful initialization
            if (retval == 0)
                subsystem_refcounts[sub]++;
            return retval;
        }
        else {
            // Already initialized, always successful
            subsystem_refcounts[sub]++;
            return 0;
        }
    }

    void quit(Subsystem::Type sub) {
        assert(sub < Subsystem::NumSubsystems);
        assert(subsystem_refcounts[sub] > 0);

        subsystem_refcounts[sub]--;
        if (subsystem_refcounts[sub] == 0)
            SDL_QuitSubSystem(SDLSubsystem(sub));
    }

    // Whether SDL is currently initialized
    bool initialized;
    // Refcounts on each subsystem
    int32 subsystem_refcounts[Subsystem::NumSubsystems];
};
SafeInitCleanup gSafeInitCleanup;

} // namespace

int SIRIKATA_SDL_FUNCTION_EXPORT InitializeSubsystem(Subsystem::Type sub) {
    return gSafeInitCleanup.initialize(sub);
}

void SIRIKATA_SDL_FUNCTION_EXPORT QuitSubsystem(Subsystem::Type sub) {
    gSafeInitCleanup.quit(sub);
}

} // namespace SDL
} // namespace Sirikata

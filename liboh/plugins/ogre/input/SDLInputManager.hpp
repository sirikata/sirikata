/*  Sirikata liboh -- Ogre Graphics Plugin
 *  SDLInputManager.hpp
 *
 *  Copyright (c) 2009, Daniel Reiter Horn
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Sirikata nor the names of its contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
extern "C" typedef struct SDL_Surface SDL_Surface;
extern "C" typedef Sirikata::uint32 SDL_WindowID;
extern "C" typedef void* SDL_GLContext;
#include <task/EventManager.hpp>
#include "InputManager.hpp"
namespace Sirikata { namespace Input {

class InputDevice;
class SDLKeyboard;
class SDLMouse;
class SDLJoystick;
typedef std::tr1::shared_ptr<InputDevice> InputDevicePtr;
typedef std::tr1::weak_ptr<InputDevice> InputDeviceWPtr;
typedef std::tr1::shared_ptr<SDLMouse> SDLMousePtr;
typedef std::tr1::shared_ptr<SDLJoystick> SDLJoystickPtr;
typedef std::tr1::shared_ptr<SDLKeyboard> SDLKeyboardPtr;


class SDLInputManager : public InputManager {
    SDL_WindowID mWindowID;
    SDL_GLContext mWindowContext;
    std::vector<SDLKeyboardPtr> mKeys;
    std::vector<SDLMousePtr> mMice;
    std::vector<SDLJoystickPtr> mJoy;
    unsigned int mWidth, mHeight;
    bool mHasKeyboardFocus;
    static int modifiersFromSDL(int sdlMod);
public:
    OptionValue*mDragDeadband;
    OptionValue*mDragMultiplier;
    OptionValue*mWorldScale;
    OptionValue*mAxisToRadians;
    OptionValue*mWheelToAxis;
    OptionValue*mRelativeMouseToAxis;
    OptionValue*mJoyBallToAxis;

    void getWindowSize(unsigned int &width, unsigned int &height) {
        width = this->mWidth;
        height = this->mHeight;
    }
    SDLInputManager(unsigned int width,
                    unsigned int height,
                    bool fullscreen,
                    int ogrePixelFmt, // fixme: which one do we use?
                    bool grabCursor,
                    void *&currentWindowData);
    bool tick(Time currentTime, Duration frameTime);

	void filesDropped(const std::vector<std::string> &files);

    virtual bool isModifierDown(int modifier) const;
    virtual bool isCapsLockDown() const;
    virtual bool isNumLockDown() const;
    virtual bool isScrollLockDown() const;

    virtual ~SDLInputManager();
};
} }

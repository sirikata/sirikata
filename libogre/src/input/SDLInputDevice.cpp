/*  Sirikata Input Plugin -- plugins/input
 *  SDLInputDevice.cpp
 *
 *  Copyright (c) 2009, Patrick Reiter Horn
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

#include <sirikata/core/util/Standard.hh>
#include <sirikata/ogre/task/Event.hpp>
#include <sirikata/ogre/task/EventManager.hpp>
#include <sirikata/ogre/input/SDLInputManager.hpp>
#include <sirikata/ogre/input/SDLInputDevice.hpp>
#include <sirikata/ogre/input/InputEvents.hpp>

#include <SDL.h>
#include <SDL_video.h>
#include <SDL_syswm.h>
#include <SDL_events.h>
#include <SDL_mouse.h>

#include <boost/lexical_cast.hpp>
#include <SDL_keysym.h>

/*
#if SIRIKATA_PLATFORM == PLATFORM_MAC
#include <CGRemoteOperation.h>
#endif
*/
#ifdef nil
#undef nil
#endif

namespace Sirikata {
namespace Input {

using Task::EventPtr;
using Task::GenEventManager;

// HACK: Constant to convert uncalibrated axes to floating point.
// We need a way to set calibration values for each axis.


SDLMouse::SDLMouse(SDLInputManager *manager, unsigned int which) : mWhich(which) {
    unsigned int wid,hei;
    manager->getWindowSize(wid,hei);
    float maxsize = sqrt((float)(wid*wid+hei*hei));
    float drag_deadband (manager->dragDeadBand());
    setDragDeadband(2*drag_deadband/maxsize);
    setName(SDL_GetMouseName(which));

    // SDL has no API to query the number of buttons. The exact value doesn't matter.
    mNumButtons = 5; // left, middle, right, Back, Forward
}

SDLMouse::~SDLMouse() {
}

int SDLMouse::getNumButtons() const {
    return mNumButtons;
}
std::string SDLMouse::getButtonName(unsigned int button) const {
    switch (button) {
      case SDL_BUTTON_LEFT:
        return "Left Button";
      case SDL_BUTTON_MIDDLE:
        return "Middle Button";
      case SDL_BUTTON_RIGHT:
        return "Right Button";
    }
    {
        std::ostringstream os;
        os << "Button " << button;
        return os.str();
    }
}

void SDLMouse::setRelativeMode(bool enabled) {
    int oldmouse = SDL_SelectMouse(mWhich);
    SDL_ShowCursor(enabled? 1 : 0);
    SDL_SetRelativeMouseMode(mWhich, enabled ? SDL_TRUE : SDL_FALSE);
/*#if SIRIKATA_PLATFORM == PLATFORM_MAC
    CGAssociateMouseAndMouseCursorPosition(!enabled);
#endif*/
    SDL_SelectMouse(oldmouse);
}

void SDLMouse::fireMotion(const SDLMousePtr &thisptr,
                          SDLInputManager *em,
                          const struct SDL_MouseMotionEvent &event) {
    unsigned int screenwid, screenhei;
    em->getWindowSize(screenwid, screenhei);
    AxisValue xPctFromCenter = AxisValue::from01(event.x / float(screenwid));
    AxisValue yPctFromCenter = AxisValue::from01(1-(event.y / float(screenhei)));

    bool changedx = (getAxis(AXIS_CURSORX) != xPctFromCenter);
    bool changedy = (getAxis(AXIS_CURSORY) != yPctFromCenter);
    if (mRelativeMode) {
        // negate y axis since coordinates start at bottom-left.
        float to_axis(em->relativeMouseToAxis());
        fireAxis(thisptr, em, AXIS_RELX, AxisValue::fromCentered(to_axis*event.xrel));
        fireAxis(thisptr, em, AXIS_RELY, AxisValue::fromCentered(-to_axis*event.yrel));
        // drag events still get fired...
        firePointerMotion(thisptr, em, event.xrel/float(screenwid),
                          -event.yrel/float(screenhei), event.cursor,
                          event.pressure, event.pressure_min, event.pressure_max);
    } else {
        fireAxis(thisptr, em, AXIS_CURSORX, xPctFromCenter);
        fireAxis(thisptr, em, AXIS_CURSORY, yPctFromCenter);
        if (changedx || changedy) {
            firePointerMotion(thisptr, em, xPctFromCenter.getCentered(),
                              yPctFromCenter.getCentered(), event.cursor,
                              event.pressure, event.pressure_min, event.pressure_max);
        }
    }
    if (event.pressure_max) {
        fireAxis(thisptr, em, PRESSURE, AxisValue::fromCentered((float)event.pressure / (float)event.pressure_max));
    }

    // "For future use"
    //fireAxis(em, CURSORZ, event.z);
    //fireAxis(em, TILT, event.pressure);
    //fireAxis(em, ROTATION, event.pressure);
}
void SDLMouse::fireWheel(const SDLMousePtr &thisptr,
                         SDLInputManager *em, int xrel, int yrel) {
    float to_axis (em->wheelToAxis());
    SILOG(input,detailed,"WHEEL: " << xrel << "; " << yrel);
    fireAxis(thisptr, em, WHEELX, AxisValue::fromCentered(to_axis*xrel));
    fireAxis(thisptr, em, WHEELY, AxisValue::fromCentered(to_axis*yrel));
}

SDLJoystick::SDLJoystick(unsigned int whichDevice)
    : mWhich(whichDevice) {
    this->mJoy = SDL_JoystickOpen(whichDevice);
    if (mJoy == NULL) {
        // analog joystick?
        mNumBalls = 0;
        mNumButtons = 4;
        mNumHats = 0;
        mNumGeneralAxes = 4;
        SILOG(input,error,"Unable to open joystick " << whichDevice << "!");
    } else {
        mNumGeneralAxes = SDL_JoystickNumAxes(mJoy);
        mNumBalls = SDL_JoystickNumBalls(mJoy);
        mNumButtons = SDL_JoystickNumButtons(mJoy);
        mNumHats = SDL_JoystickNumHats(mJoy);
        mButtonNames.resize(mNumButtons);
    }
}

SDLJoystick::~SDLJoystick() {
    if (mJoy) {
        SDL_JoystickClose(mJoy);
    }
}

int SDLJoystick::getNumButtons() const {
    return mNumButtons + HAT_MAX*mNumHats;
}

unsigned int SDLJoystick::getNumAxes() const {
    return mNumGeneralAxes + 2*mNumBalls;
}

std::string SDLJoystick::getAxisName(unsigned int axis) const {
    if (axis == 0) {
        return "X";
    } else if (axis == 1) {
        return "Y";
    } else if (axis > mNumGeneralAxes) {
        unsigned int ballaxis = axis - mNumGeneralAxes;
        unsigned int ballnum = ballaxis/2;
        const char *axisname = ((ballnum%2) == 0) ? " X" : " Y";
        std::ostringstream os;
        os << "Trackball " << ballnum << axisname;
        return os.str();
    } else {
        std::ostringstream os;
        os << "Axis " << axis;
        return os.str();
    }
}

void SDLJoystick::fireHat(const SDLJoystickPtr &thisptr,
                          GenEventManager *em, unsigned int num, int val) {
    this->fireButton(thisptr, em, mNumButtons + HAT_MAX*num + HAT_UP, !!(val & SDL_HAT_UP));
    this->fireButton(thisptr, em, mNumButtons + HAT_MAX*num + HAT_RIGHT, !!(val & SDL_HAT_RIGHT));
    this->fireButton(thisptr, em, mNumButtons + HAT_MAX*num + HAT_DOWN, !!(val & SDL_HAT_DOWN));
    this->fireButton(thisptr, em, mNumButtons + HAT_MAX*num + HAT_LEFT, !!(val & SDL_HAT_LEFT));
}
void SDLJoystick::fireBall(const SDLJoystickPtr &thisptr,
                          SDLInputManager *em, unsigned int ballNumber, int xrel, int yrel) {
    float to_axis (em->joyBallToAxis());
    fireAxis(thisptr, em, mNumGeneralAxes + 2*ballNumber, AxisValue::fromCentered(to_axis*xrel));
    fireAxis(thisptr, em, mNumGeneralAxes + 2*ballNumber + 1, AxisValue::fromCentered(to_axis*yrel));
}

std::string SDLJoystick::getButtonName(unsigned int button) const {
    if (button < mButtonNames.size()) {
        if (!mButtonNames[button].empty()) {
            return mButtonNames[button];
        }
    }
    std::ostringstream os;
    if (button >= mNumButtons) {
        int hatbutton = button - mNumButtons;
        os << "Digital Hat " << (hatbutton/HAT_MAX) << " ";
        int direction = hatbutton%HAT_MAX;
        switch (hatbutton) {
          case HAT_UP:
            os << "Up";
            break;
          case HAT_DOWN:
            os << "Up";
            break;
          case HAT_LEFT:
            os << "Left";
            break;
          case HAT_RIGHT:
            os << "Right";
            break;
        }
        return os.str();
    } else {
        os << "Button " << button;
        return os.str();
    }
}

void SDLJoystick::setButtonName(unsigned int button, std::string name) {
    if (button < mButtonNames.size()) {
        mButtonNames[button] = name;
    }
}



SDLKeyboard::SDLKeyboard(unsigned int which) : mWhich(which) {
    if (which == 0) {
        setName("Primary Keyboard");
    } else {
        std::ostringstream os;
        os << "Keyboard " << which;
        setName(os.str());
    }

}

SDLKeyboard::~SDLKeyboard() {
}


std::string SDLKeyboard::getButtonName(unsigned int button) const {
    const char *keyname = SDL_GetScancodeName((SDL_scancode)button);
    if (keyname) {
        return keyname;
    } else {
        std::ostringstream ostr;
        ostr << '#' << button;
        return ostr.str();
    }
}


static std::map<Input::KeyButton, String> ScancodesToStrings;
static std::map<String, Input::KeyButton> StringsToScancodes;

static void init_button_conversion_maps();
void ensure_initialized() {
    static bool initialized = false;
    if (!initialized) {
        initialized = true;
        init_button_conversion_maps();
    }
}

#define INIT_SCANCODE_STRING_MAP(X, STR)                     \
    ScancodesToStrings[SDL_SCANCODE_##X] = #STR;             \
    StringsToScancodes[#STR] = SDL_SCANCODE_##X

static void init_button_conversion_maps() {
    INIT_SCANCODE_STRING_MAP(A, a); INIT_SCANCODE_STRING_MAP(B, b); INIT_SCANCODE_STRING_MAP(C, c);
    INIT_SCANCODE_STRING_MAP(D, d); INIT_SCANCODE_STRING_MAP(E, e); INIT_SCANCODE_STRING_MAP(F, f);
    INIT_SCANCODE_STRING_MAP(G, g); INIT_SCANCODE_STRING_MAP(H, h); INIT_SCANCODE_STRING_MAP(I, i);
    INIT_SCANCODE_STRING_MAP(J, j); INIT_SCANCODE_STRING_MAP(K, k); INIT_SCANCODE_STRING_MAP(L, l);
    INIT_SCANCODE_STRING_MAP(M, m); INIT_SCANCODE_STRING_MAP(N, n); INIT_SCANCODE_STRING_MAP(O, o);
    INIT_SCANCODE_STRING_MAP(P, p); INIT_SCANCODE_STRING_MAP(Q, q); INIT_SCANCODE_STRING_MAP(R, r);
    INIT_SCANCODE_STRING_MAP(S, s); INIT_SCANCODE_STRING_MAP(T, t); INIT_SCANCODE_STRING_MAP(U, u);
    INIT_SCANCODE_STRING_MAP(V, v); INIT_SCANCODE_STRING_MAP(W, w); INIT_SCANCODE_STRING_MAP(X, X);
    INIT_SCANCODE_STRING_MAP(Y, y); INIT_SCANCODE_STRING_MAP(Z, z); INIT_SCANCODE_STRING_MAP(0, 0);
    INIT_SCANCODE_STRING_MAP(1, 1); INIT_SCANCODE_STRING_MAP(2, 2); INIT_SCANCODE_STRING_MAP(3, 3);
    INIT_SCANCODE_STRING_MAP(4, 4); INIT_SCANCODE_STRING_MAP(5, 5); INIT_SCANCODE_STRING_MAP(6, 6);
    INIT_SCANCODE_STRING_MAP(7, 7); INIT_SCANCODE_STRING_MAP(8, 8); INIT_SCANCODE_STRING_MAP(9, 9);

    INIT_SCANCODE_STRING_MAP(LSHIFT, lshift);
    INIT_SCANCODE_STRING_MAP(RSHIFT, rshift);
    INIT_SCANCODE_STRING_MAP(LCTRL, lctrl);
    INIT_SCANCODE_STRING_MAP(RCTRL, rctrl);
    INIT_SCANCODE_STRING_MAP(LALT, lalt);
    INIT_SCANCODE_STRING_MAP(RALT, ralt);
    INIT_SCANCODE_STRING_MAP(LGUI, lsuper);
    INIT_SCANCODE_STRING_MAP(RGUI, rsuper);
    INIT_SCANCODE_STRING_MAP(RETURN, return);
    INIT_SCANCODE_STRING_MAP(ESCAPE, escape);
    INIT_SCANCODE_STRING_MAP(BACKSPACE, back);
    INIT_SCANCODE_STRING_MAP(TAB, tab);
    INIT_SCANCODE_STRING_MAP(SPACE, space);
    INIT_SCANCODE_STRING_MAP(MINUS, minus);
    INIT_SCANCODE_STRING_MAP(EQUALS, equals);
    INIT_SCANCODE_STRING_MAP(LEFTBRACKET, [);
    INIT_SCANCODE_STRING_MAP(RIGHTBRACKET, ]);
    INIT_SCANCODE_STRING_MAP(BACKSLASH, backslash);
    INIT_SCANCODE_STRING_MAP(SEMICOLON, semicolon);
    INIT_SCANCODE_STRING_MAP(APOSTROPHE, apostrophe);
    INIT_SCANCODE_STRING_MAP(GRAVE, OEM_3);
    INIT_SCANCODE_STRING_MAP(COMMA, comma);
    INIT_SCANCODE_STRING_MAP(PERIOD, period);
    INIT_SCANCODE_STRING_MAP(SLASH, OEM_2);
    INIT_SCANCODE_STRING_MAP(CAPSLOCK, CAPITAL);
    INIT_SCANCODE_STRING_MAP(F1, F1);
    INIT_SCANCODE_STRING_MAP(F2, F2);
    INIT_SCANCODE_STRING_MAP(F3, F3);
    INIT_SCANCODE_STRING_MAP(F4, F4);
    INIT_SCANCODE_STRING_MAP(F5, F5);
    INIT_SCANCODE_STRING_MAP(F6, F6);
    INIT_SCANCODE_STRING_MAP(F7, F7);
    INIT_SCANCODE_STRING_MAP(F8, F8);
    INIT_SCANCODE_STRING_MAP(F9, F9);
    INIT_SCANCODE_STRING_MAP(F10, F10);
    INIT_SCANCODE_STRING_MAP(F11, F11);
    INIT_SCANCODE_STRING_MAP(F12, F12);
    INIT_SCANCODE_STRING_MAP(PRINTSCREEN, print);
    INIT_SCANCODE_STRING_MAP(SCROLLLOCK, scroll);
    INIT_SCANCODE_STRING_MAP(PAUSE, pause);
    INIT_SCANCODE_STRING_MAP(INSERT, insert);
    INIT_SCANCODE_STRING_MAP(HOME, home);
    INIT_SCANCODE_STRING_MAP(PAGEUP, pageup);
    INIT_SCANCODE_STRING_MAP(DELETE, delete);
    INIT_SCANCODE_STRING_MAP(END, end);
    INIT_SCANCODE_STRING_MAP(PAGEDOWN, pagedown);
    INIT_SCANCODE_STRING_MAP(RIGHT, right);
    INIT_SCANCODE_STRING_MAP(LEFT, left);
    INIT_SCANCODE_STRING_MAP(DOWN, down);
    INIT_SCANCODE_STRING_MAP(UP, up);
    INIT_SCANCODE_STRING_MAP(KP_0, insert);
    INIT_SCANCODE_STRING_MAP(KP_1, end);
    INIT_SCANCODE_STRING_MAP(KP_2, down);
    INIT_SCANCODE_STRING_MAP(KP_3, next);
    INIT_SCANCODE_STRING_MAP(KP_4, left);
    INIT_SCANCODE_STRING_MAP(KP_6, right);
    INIT_SCANCODE_STRING_MAP(KP_7, home);
    INIT_SCANCODE_STRING_MAP(KP_8, up);
    INIT_SCANCODE_STRING_MAP(KP_9, prior);
}

bool keyIsModifier(Input::KeyButton b) {
    return (b == SDL_SCANCODE_LSHIFT ||
        b == SDL_SCANCODE_RSHIFT ||
        b == SDL_SCANCODE_LCTRL ||
        b == SDL_SCANCODE_RCTRL ||
        b == SDL_SCANCODE_LALT ||
        b == SDL_SCANCODE_RALT ||
        b == SDL_SCANCODE_LGUI ||
        b == SDL_SCANCODE_RGUI);
}

String keyButtonString(Input::KeyButton b) {
    ensure_initialized();
    if (ScancodesToStrings.find(b) != ScancodesToStrings.end())
        return ScancodesToStrings[b];
    return "";
}

String keyModifiersString(Input::Modifier m) {
    if (m == Input::MOD_NONE)
        return "";
    String result;
    if (m & MOD_SHIFT) {
        result += "shift";
    }
    if (m & MOD_CTRL) {
        if (!result.empty()) result += "-";
        result += "ctrl";
    }
    if (m & MOD_ALT) {
        if (!result.empty()) result += "-";
        result += "alt";
    }
    if (m & MOD_GUI) {
        if (!result.empty()) result += "-";
        result += "super";
    }
    return result;
}

String mouseButtonString(Input::MouseButton b) {
    return boost::lexical_cast<String>(b);
}

String axisString(Input::AxisIndex i) {
    return boost::lexical_cast<String>(i);
}

Input::KeyButton keyButtonFromStrings(std::vector<String>& parts) {
    ensure_initialized();

    assert(!parts.empty());
    const String& part = *parts.begin();
    Input::KeyButton result = 0;
    if (StringsToScancodes.find(part) != StringsToScancodes.end()) {
        result = StringsToScancodes[part];
        parts.erase(parts.begin());
    }
    return result;
}

Input::Modifier keyModifiersFromStrings(std::vector<String>& parts) {
    bool more = true;
    Input::Modifier result = Input::MOD_NONE;
    while(more && !parts.empty()) {
        const String& val = *parts.begin();
        if (val == "shift")
            result |= Input::MOD_SHIFT;
        else if (val == "ctrl")
            result |= Input::MOD_CTRL;
        else if (val == "alt")
            result |= Input::MOD_ALT;
        else if (val == "super")
            result |= Input::MOD_GUI;
        else
            more = false;
        if (more) // We got something, so we need to remove it
            parts.erase(parts.begin());
    }
    return result;
}

Input::MouseButton mouseButtonFromStrings(std::vector<String>& parts) {
    assert(!parts.empty());
    Input::MouseButton result = boost::lexical_cast<Input::MouseButton>(*parts.begin());
    parts.erase( parts.begin() );
    return result;
}

Input::AxisIndex axisFromStrings(std::vector<String>& parts) {
    assert(!parts.empty());
    Input::AxisIndex result = boost::lexical_cast<Input::AxisIndex>(*parts.begin());
    parts.erase( parts.begin() );
    return result;
}

}
}

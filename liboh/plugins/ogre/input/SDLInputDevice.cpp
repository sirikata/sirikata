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

#include <util/Standard.hh>
#include <task/Event.hpp>
#include <task/EventManager.hpp>
#include "SDLInputManager.hpp"
#include "SDLInputDevice.hpp"
#include "InputEvents.hpp"

#include <SDL.h>
#include <SDL_video.h>
#include <SDL_syswm.h>
#include <SDL_events.h>
#include <SDL_mouse.h>

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
    float drag_deadband (manager->mDragDeadband->as<float>());
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

    bool changedx = (getAxis(CURSORX) != xPctFromCenter);
    bool changedy = (getAxis(CURSORY) != yPctFromCenter);
    if (mRelativeMode) {
        // negate y axis since coordinates start at bottom-left.
        float to_axis(em->mRelativeMouseToAxis->as<float>());
        fireAxis(thisptr, em, RELX, AxisValue::fromCentered(to_axis*event.xrel));
        fireAxis(thisptr, em, RELY, AxisValue::fromCentered(-to_axis*event.yrel));
        // drag events still get fired...
        firePointerMotion(thisptr, em, 2*event.xrel/float(screenwid),
                          -2*event.yrel/float(screenhei), event.cursor,
                          event.pressure, event.pressure_min, event.pressure_max);
    } else {
        fireAxis(thisptr, em, CURSORX, xPctFromCenter);
        fireAxis(thisptr, em, CURSORY, yPctFromCenter);
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
    float to_axis (em->mWheelToAxis->as<float>());
    SILOG(input,debug,"WHEEL: " << xrel << "; " << yrel);
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
    float to_axis (em->mJoyBallToAxis->as<float>());
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

}
}

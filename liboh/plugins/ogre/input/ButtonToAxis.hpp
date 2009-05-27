/*  Sirikata Input Plugin -- plugins/input
 *  ButtonToAxis.hpp
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

#ifndef SIRIKATA_INPUT_ButtonToAxis_HPP__
#define SIRIKATA_INPUT_ButtonToAxis_HPP__

#include "InputDevice.hpp"

namespace Sirikata {
namespace Input {

/** Mostly for hatswitches or d-pad's on a joystick. */
class AxisToButton : public InputDevice {
    struct HatButton {
        AxisValue mLow;
        AxisValue mHigh;
    };
    std::vector <HatButton> mButtons;

    InputDevicePtr mParentDevice;
    int mParentAxis;

public:
    AxisToButton (const std::string &name, const InputDevicePtr &parent, int parentAxis) 
        : mParentDevice(parent), mParentAxis(parentAxis) {
        setName(name);
    }

    virtual std::string getButtonName(unsigned int num) const {
        std::ostringstream os ("[" + mParentDevice->getAxisName(mParentAxis) + " ");
        const HatButton & button = mButtons[num];
        if (button.mLow.isNegative() && button.mHigh.isPositive()) {
            os << "Centered";
        } else {
            bool lessZero = button.mHigh.isNegative();
            AxisValue extreme = button.mHigh;
            if (lessZero) {
                extreme = button.mLow;
            }
            os << ((int)(extreme.getCentered()*100)) << "%";
        }
        os << "]";
        return os.str();
    }

    virtual int getNumButtons() const {
        return mButtons.size();
    }
    virtual unsigned int getNumAxes() const {
        return 0;
    }
    virtual std::string getNumAxisName(unsigned int i) const {
        return std::string();
    }

    void addButton(AxisValue low, AxisValue high) {
        HatButton hb = {low, high};
        mButtons.push_back(hb);
    }

};

/** Make arrow keys on a keyboard emulate a mouse/axis. */
class ButtonToAxis : public InputDevice {
    typedef std::map <int, float> AxisMap;
    AxisMap mAxisPoints;
    InputDevicePtr mParentDevice;
    int mParentButton;
public:
    ButtonToAxis (const std::string &name, const InputDevicePtr &parent, int parentButton) 
        : mParentDevice(parent), mParentButton(parentButton) {
        setName(name);
    }

    virtual std::string getAxisName(unsigned int axis) {
        std::string os = "[" + mParentDevice->getName() + " ";
        bool first = true;
        for (AxisMap::const_iterator iter = mAxisPoints.begin(); iter != mAxisPoints.end(); ++iter) {
            if (!first) {
                os += '/';
            }
            first = false;
            os += mParentDevice->getButtonName((*iter).first);
        }
        os += "]";
        return os;
    }
    virtual unsigned int getNumAxes() const {
        return 1;
    }
    virtual int getNumButtons() const {
        return 0;
    }
    virtual std::string getButtonName(unsigned int button) const {
        return std::string();
    }

};

#endif

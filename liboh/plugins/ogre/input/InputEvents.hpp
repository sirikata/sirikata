/*  Sirikata liboh -- Ogre Graphics Plugin
 *  InputEvents.hpp
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

#ifndef SIRIKATA_INPUT_InputEvents_HPP__
#define SIRIKATA_INPUT_InputEvents_HPP__

#include "InputDevice.hpp"

namespace Sirikata {
namespace Input {

using Task::IdPair;
class InputEvent : public Task::Event {
    InputDeviceWPtr mDevice;

public:
    InputEvent(const InputDeviceWPtr &dev, const IdPair &id) 
        : Task::Event(id), mDevice(dev) {
    }

    static IdPair::Secondary getSecondaryId(const InputDevicePtr &dev) {
        std::ostringstream os;
        os << &(*dev);
        return IdPair::Secondary(os.str());
    }
    
    virtual ~InputEvent() {}

    InputDevicePtr getDevice() const {
        return mDevice.lock();
    }
};

class ButtonEvent : public InputEvent {
public:
    typedef InputDevice::Modifier Modifier;

    bool mPressed;
    unsigned int mButton;
    Modifier mModifier;

    static IdPair::Secondary getSecondaryId(int keycode,
                                            Modifier mod,
                                            const InputDevicePtr &device) {
        std::ostringstream os;
        os << keycode << ',' << &(*device) << " mod " << mod;
        return IdPair::Secondary(os.str());
    }
    virtual ~ButtonEvent(){}
    ButtonEvent(const Task::IdPair::Primary&primary, 
                const InputDevicePtr &dev,
                bool pressed, unsigned int key, Modifier mod)
        : InputEvent(dev, IdPair(primary,
                       getSecondaryId(key, mod, dev))),
         mPressed(pressed),
         mButton(key),
         mModifier(mod) {
    }
};
class ButtonPressed :public ButtonEvent {
public:
    static const IdPair::Primary& getEventId(){
        static IdPair::Primary retval("ButtonPressed");
        return retval;
    }
    ButtonPressed(const InputDevicePtr &dev, unsigned int key, Modifier mod)
        : ButtonEvent(getEventId(), dev, true, key, mod) {
    }
    virtual ~ButtonPressed(){}
};
class ButtonReleased :public ButtonEvent {
public:
    static const IdPair::Primary& getEventId(){
        static IdPair::Primary retval("ButtonReleased");
        return retval;
    }
    ButtonReleased(const InputDevicePtr &dev, unsigned int key, Modifier mod)
        : ButtonEvent(getEventId(), dev, false, key, mod) {
    }
    virtual ~ButtonReleased(){}
};
class ButtonDown :public ButtonEvent {
public:
    static const IdPair::Primary& getEventId(){
        static IdPair::Primary retval("ButtonDown");
        return retval;
    }
    ButtonDown(const InputDevicePtr &dev, unsigned int key, Modifier mod)
        : ButtonEvent(getEventId(), dev, true, key, mod) {
    }
    virtual ~ButtonDown(){}
};

class AxisEvent: public InputEvent {
public:
    unsigned int mAxis;
    AxisValue mValue;

    static const IdPair::Primary &getEventId() {
        static IdPair::Primary retval("AxisEvent");
        return retval;
    }

    static IdPair::Secondary getSecondaryId(const InputDevicePtr &dev, unsigned int axis) {
        std::ostringstream os;
        os << &(*dev) << ',' << axis;
        return IdPair::Secondary(os.str());
    }

    AxisEvent(const InputDevicePtr &dev, unsigned int axis, AxisValue value)
        : InputEvent(dev, IdPair(getEventId(), getSecondaryId(dev, axis))) {
        mAxis = axis;
        mValue = value;
    }
};

class TextInputEvent: public InputEvent {
public:
    std::string mText;

    static const IdPair::Primary &getEventId() {
        static IdPair::Primary retval("TextInputEvent");
        return retval;
    }

    TextInputEvent(const InputDevicePtr &dev, char *text) 
        : InputEvent(dev, IdPair(getEventId(), getSecondaryId(dev))),
                                 mText(text) {
    }
};

class MouseEvent: public InputEvent {
public:
    int mX;
    int mY;
    int mCursorType;

    MouseEvent(const IdPair &id,
               const PointerDevicePtr &dev, 
               int x, int y, int cursorType)
        : InputEvent(dev, id) {
        mX = x;
        mY = y;
        mCursorType = cursorType;
    }

    PointerDevicePtr getDevice() const {
        return std::tr1::static_pointer_cast<PointerDevice>(
            InputEvent::getDevice());
    }
};

class MouseHoverEvent: public MouseEvent {
public:
    static const IdPair::Primary &getEventId() {
        static IdPair::Primary retval("MouseHoverEvent");
        return retval;
    }

    MouseHoverEvent(const PointerDevicePtr &dev, 
               int x, int y, int cursorType)
        : MouseEvent(IdPair(getEventId(), getSecondaryId(dev)), dev, x, y, cursorType) {
    }
};

class MouseDownEvent: public MouseEvent {
public:
    int mButton;
    int mXStart;
    int mYStart;

    static IdPair::Secondary getSecondaryId(int button,
                                            const InputDevicePtr &device) {
        std::ostringstream os;
        os << &(*device) << "," << button;
        return IdPair::Secondary(os.str());
    }

    MouseDownEvent(const IdPair::Primary &priId,
                    const PointerDevicePtr &dev, 
                    int xstart, int ystart, int xend, int yend, 
                    int cursorType, int button)
        : MouseEvent(IdPair(priId, getSecondaryId(button, dev)), 
                     dev, xend, yend, cursorType) {
        mButton = button;
        mXStart = xstart;
        mYStart = ystart;
    }
};

class MouseClickEvent: public MouseDownEvent {
public:
    bool mDrag;

    static const IdPair::Primary getEventId() {
        static IdPair::Primary retval("MouseClickEvent");
        return retval;
    }

    MouseClickEvent(const PointerDevicePtr &dev, 
                    int xstart, int ystart, int xend, int yend, 
                    int cursorType, int button)
        : MouseDownEvent(getEventId(), dev, xstart, ystart, 
                         xend, yend, cursorType, button) {
        mDrag = !(xstart == xend && ystart == yend);
    }
};

class MouseDragEvent: public MouseDownEvent {
public:
    int mPressure;
    int mPressureMax;
    int mPressureMin;

    static const IdPair::Primary &getEventId() {
        static IdPair::Primary retval("MouseDragEvent");
        return retval;
    }

    MouseDragEvent(const PointerDevicePtr &dev, 
                   int xstart, int ystart, int xend, int yend,
                   int cursorType, int button,
                   int pressure, int pressureMin, int pressureMax)
        : MouseDownEvent(getEventId(), dev, xstart, ystart, 
                         xend, yend, cursorType, button) {
        mPressure = pressure;
        mPressureMin = pressureMin;
        mPressureMax = pressureMax;
    }
};



class WindowEvent: public Task::Event {
public:
    static IdPair::Primary Shown;
    static IdPair::Primary Hidden;
    static IdPair::Primary Exposed;
    static IdPair::Primary Moved;
    static IdPair::Primary Resized;
    static IdPair::Primary Minimized;
    static IdPair::Primary Maximized;
    static IdPair::Primary Restored;
    static IdPair::Primary MouseEnter;
    static IdPair::Primary MouseLeave;
    static IdPair::Primary FocusGained;
    static IdPair::Primary FocusLost;
    static IdPair::Primary Quit;

    int mData1;
    int mData2;

    WindowEvent(const IdPair::Primary &priId, int data1, int data2)
        : Task::Event(IdPair(priId, IdPair::Secondary::null())) {
        mData1 = data1;
        mData2 = data2;
    }

    int getX() const {
        return mData1;
    }
    int getY() const {
        return mData2;
    }
};

#endif

}
}

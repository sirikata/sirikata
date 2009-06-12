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

/** Base class for all input events. Call getDevice() to
    retrieve the specific device that fired the event,
    and to determine the name of any buttons/axes, or the
    the meaning of each modifier/scancode. */
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

/** Base class for keyboard/mouse/joystick button events. */
class ButtonEvent : public InputEvent {
public:
    typedef InputDevice::Modifier Modifier;

    bool mPressed; ///< If the event is ButtonPressed or ButtonReleased
    unsigned int mButton; ///< Which scancode (keyboard)
    Modifier mModifier; ///< OR of all modifier codes (defined by SDL)

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
/** Fired whenever a button has been pushed down. This event is only
    fired when the button is pressed.  Note that both a ButtonReleased
    and a ButtonPressed event will be fired if modifiers change. */
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

/** Fired whenever a button is no longer held down. */
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

/** In theory, this might be fired as long as a button is held down.
    In reality, you should be using TextInputEvent for repeated characters,
    and for other cases, it may be more appropriate to use ButtonToAxis.

    This event is currently never fired unless there is a reason it is needed. */
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

/** Upon changing the value of a mouse/joystick axis.  NOTE: This does
    not take into account relative (per-frame) versus absolute axes.

    For example, the mouse wheel is fired only once upon scrolling a few lines.
    On the other hand, a joystick is constantly fired whenever the value changes. */
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

/** SDL event fired as soon as textual input has been entered.
    Usually this corresponds to one ButtonPress/ButtonRelease event.
    However, there are some cases where they differ.  This should
    also make use of the operating system's IME support. Either way,
    mText is always a UTF-8 formatted string, not a scancode.

    As an example, X11 contains a special key called the MultiKey, which
    contains 2-key sequences for complicated characters, like Multi+a+e
    produces the "ae" digraph.  mText will contain the single UTF-8
    sequence for the specific letter produced.

    FIXME: How are Backspace and Delete handled? */
class TextInputEvent: public InputEvent {
public:
    std::string mText; ///< UTF-8 formatted string to be appended to an input box.

    static const IdPair::Primary &getEventId() {
        static IdPair::Primary retval("TextInputEvent");
        return retval;
    }

    TextInputEvent(const InputDevicePtr &dev, char *text) 
        : InputEvent(dev, IdPair(getEventId(), getSecondaryId(dev))),
                                 mText(text) {
    }
};

/** Base class for all pointer motion events. */
class MouseEvent: public InputEvent {
public:
    float mX; ///< X coordinate, from -1 (left) to 1 (right), same as AxisEvent
    float mY; ///< Y coordinate, from -1 (bottom) to 1 (top), same as AxisEvent
    int mCursorType; ///< Platform-dependent value as defined by SDL.

    MouseEvent(const IdPair &id,
               const PointerDevicePtr &dev, 
               float x, float y, int cursorType)
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

/** Event called when the cursor moves, and no buttons are down. */
class MouseHoverEvent: public MouseEvent {
public:
    static const IdPair::Primary &getEventId() {
        static IdPair::Primary retval("MouseHoverEvent");
        return retval;
    }

    MouseHoverEvent(const PointerDevicePtr &dev, 
               float x, float y, int cursorType)
        : MouseEvent(IdPair(getEventId(), getSecondaryId(dev)), dev, x, y, cursorType) {
    }
};

/** Base class for events involving a mouse click. */
class MouseDownEvent: public MouseEvent {
public:
    int mButton; ///< The button this event is about.
    float mXStart; ///< X coordinate when the mouse button was first pressed, -1 to 1
    float mYStart; ///< Y coordinate when the mouse button was pressed, -1 to 1
    float mLastX;
    float mLastY;
    int mPressure; ///< Pressure value as defined by SDL.
    int mPressureMax;
    int mPressureMin;

    /** mX - mXStart: Returns a value between -1 and 1 */
    float deltaX() const {
        return (mX - mXStart)/2.;
    }

    /** mY - mYStart: Returns a value between -1 and 1 */
    float deltaY() const {
        return (mY - mYStart)/2.;
    }

    float deltaLastY() const {
        return (mY - mLastY)/2.;
    }

    float deltaLastX() const {
        return (mX - mLastX)/2.;
    }

    static IdPair::Secondary getSecondaryId(int button) {
        std::ostringstream os;
        os << button;
        return IdPair::Secondary(os.str());
    }

    MouseDownEvent(const IdPair::Primary &priId,
                    const PointerDevicePtr &dev, 
                    float xstart, float ystart, float xend, float yend,
                    float lastx, float lasty,
                    int cursorType, int button,
                    int pressure, int pressureMin, int pressureMax)
        : MouseEvent(IdPair(priId, getSecondaryId(button)),
                     dev, xend, yend, cursorType) {
        mButton = button;
        mXStart = xstart;
        mYStart = ystart;
        mLastX = lastx;
        mLastY = lasty;
        mPressure = pressure;
        mPressureMin = pressureMin;
        mPressureMax = pressureMax;
    }
};

/** Event when the mouse was clicked (pressed and released
    without moving) */
class MouseClickEvent: public MouseDownEvent {
public:

    static const IdPair::Primary getEventId() {
        static IdPair::Primary retval("MouseClickEvent");
        return retval;
    }

    MouseClickEvent(const PointerDevicePtr &dev, 
                    float x, float y,
                    int cursorType, int button)
        : MouseDownEvent(getEventId(), dev, x, y, x, y, x, y,
                         cursorType, button, 0, 0, 0) {
    }
};

/** Event when the mouse was dragged. If this event fires, then
    you will not get a MouseClickEvent. */
class MouseDragEvent: public MouseDownEvent {
public:
    /** The three types of drag events. The START event will only be
        triggered once the motion is determined to be a drag (exceeded
        some number of pixels). The END event happens at the time the
        mouse button is released (mPressure == 0) */
    enum DragType { START, DRAG, END } mType;

    static const IdPair::Primary &getEventId() {
        static IdPair::Primary retval("MouseDragEvent");
        return retval;
    }

    MouseDragEvent(const PointerDevicePtr &dev, 
                   DragType type,
                   float xstart, float ystart, float xend, float yend,
                   float lastx, float lasty,
                   int cursorType, int button,
                   int pressure, int pressureMin, int pressureMax)
        : MouseDownEvent(getEventId(), dev, xstart, ystart, 
                         xend, yend, lastx, lasty, cursorType, button,
                         pressure, pressureMin, pressureMax) {
        mType = type;
    }
};

/** Various SDL-specific events relating to the status of the
    top-level window. */
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

class DragAndDropEvent : public Task::Event {
public:
    static IdPair::Primary Id;
    static IdPair getId() {
        return Task::IdPair(Id, IdPair::Secondary::null());
    }
    int mXCoord;
    int mYCoord;
    std::vector<std::string> mFilenames;
    DragAndDropEvent(const std::vector<std::string>&files, int x=0, int y=0)
        : Task::Event(getId()), mXCoord(x), mYCoord(y), mFilenames(files) {
    }
};

#endif

}
}

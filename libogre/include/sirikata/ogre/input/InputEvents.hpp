/*  Sirikata libproxyobject -- Ogre Graphics Plugin
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

#include <sirikata/ogre/input/InputDevice.hpp>

namespace Sirikata {

namespace Graphics {
class WebView;
}

namespace Input {

using Graphics::WebView;

/** Base class for all input events. Call getDevice() to
    retrieve the specific device that fired the event,
    and to determine the name of any buttons/axes, or the
    the meaning of each modifier/scancode. */
class SIRIKATA_OGRE_EXPORT InputEvent {
    InputDeviceWPtr mDevice;

public:
    InputEvent(const InputDeviceWPtr &dev)
        : mDevice(dev)
    {
    }
    virtual ~InputEvent() {}

    InputDevicePtr getDevice() const {
        return mDevice.lock();
    }
};
typedef std::tr1::shared_ptr<InputEvent> InputEventPtr;

/** Base class for keyboard/mouse/joystick button events. */
class SIRIKATA_OGRE_EXPORT ButtonEvent : public InputEvent {
public:
    KeyEvent mEvent; ///< If the event is ButtonPressed or ButtonReleased
    KeyButton mButton; ///< Which scancode (keyboard)
    Modifier mModifier; ///< OR of all modifier codes (defined by SDL)

    virtual ~ButtonEvent(){}
    ButtonEvent(const InputDevicePtr &dev,
        KeyEvent event, unsigned int key, Modifier mod)
        : InputEvent(dev),
        mEvent(event),
        mButton(key),
        mModifier(mod)
        {
    }

    // Indicates if the button was in the depressed state.
    bool pressed() {
        return ( (mEvent == KEY_PRESSED) || (mEvent == KEY_DOWN) || (mEvent == KEY_REPEATED) );
    }
    // Indicates if the button was actively pressed, i.e. was pushed down
    // instead of just held down
    bool activelyPressed() {
        return ( (mEvent == KEY_PRESSED) || (mEvent == KEY_DOWN) );
    }
};
typedef std::tr1::shared_ptr<ButtonEvent> ButtonEventPtr;

/** Fired whenever a button has been pushed down. This event is only
    fired when the button is pressed.  Note that both a ButtonReleased
    and a ButtonPressed event will be fired if modifiers change. */
class SIRIKATA_OGRE_EXPORT ButtonPressed :public ButtonEvent {
public:
    ButtonPressed(const InputDevicePtr &dev, unsigned int key, Modifier mod)
        : ButtonEvent(dev, KEY_PRESSED, key, mod) {
    }
    virtual ~ButtonPressed(){}
};
typedef std::tr1::shared_ptr<ButtonPressed> ButtonPressedEventPtr;

/** Fired on repeats -- whenever a button has been pushed and held
 * down.
 */
class SIRIKATA_OGRE_EXPORT ButtonRepeated :public ButtonEvent {
public:
    ButtonRepeated(const InputDevicePtr &dev, unsigned int key, Modifier mod)
        : ButtonEvent(dev, KEY_REPEATED, key, mod) {
    }
    virtual ~ButtonRepeated(){}
};
typedef std::tr1::shared_ptr<ButtonRepeated> ButtonRepeatedEventPtr;

/** Fired whenever a button is no longer held down. */
class SIRIKATA_OGRE_EXPORT ButtonReleased :public ButtonEvent {
public:
    ButtonReleased(const InputDevicePtr &dev, unsigned int key, Modifier mod)
        : ButtonEvent(dev, KEY_RELEASED, key, mod) {
    }
    virtual ~ButtonReleased(){}
};
typedef std::tr1::shared_ptr<ButtonReleased> ButtonReleasedEventPtr;

/** In theory, this might be fired as long as a button is held down.
    In reality, you should be using TextInputEvent for repeated characters,
    and for other cases, it may be more appropriate to use ButtonToAxis.

    This event is currently never fired unless there is a reason it is needed. */
class SIRIKATA_OGRE_EXPORT ButtonDown :public ButtonEvent {
public:
    ButtonDown(const InputDevicePtr &dev, unsigned int key, Modifier mod)
        : ButtonEvent(dev, KEY_DOWN, key, mod) {
    }
    virtual ~ButtonDown(){}
};
typedef std::tr1::shared_ptr<ButtonDown> ButtonDownEventPtr;

/** Upon changing the value of a mouse/joystick axis.  NOTE: This does
    not take into account relative (per-frame) versus absolute axes.

    For example, the mouse wheel is fired only once upon scrolling a few lines.
    On the other hand, a joystick is constantly fired whenever the value changes. */
class SIRIKATA_OGRE_EXPORT AxisEvent: public InputEvent {
public:
    AxisIndex mAxis;
    AxisValue mValue;

    AxisEvent(const InputDevicePtr &dev, unsigned int axis, AxisValue value)
        : InputEvent(dev),
        mAxis(axis),
        mValue(value)
    {
    }
};
typedef std::tr1::shared_ptr<AxisEvent> AxisEventPtr;

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
class SIRIKATA_OGRE_EXPORT TextInputEvent: public InputEvent {
public:
    std::string mText; ///< UTF-8 formatted string to be appended to an input box.

    TextInputEvent(const InputDevicePtr &dev, char *text)
        : InputEvent(dev),
                                 mText(text) {
    }
};
typedef std::tr1::shared_ptr<TextInputEvent> TextInputEventPtr;

/** Base class for all pointer motion events. */
class SIRIKATA_OGRE_EXPORT MouseEvent: public InputEvent {
public:
    float mX; ///< X coordinate, from -1 (left) to 1 (right), same as AxisEvent
    float mY; ///< Y coordinate, from -1 (bottom) to 1 (top), same as AxisEvent
    int mCursorType; ///< Platform-dependent value as defined by SDL.

    MouseEvent(const PointerDevicePtr &dev,
               float x, float y, int cursorType)
        : InputEvent(dev) {
        mX = x;
        mY = y;
        mCursorType = cursorType;
    }

    PointerDevicePtr getDevice() const {
        return std::tr1::static_pointer_cast<PointerDevice>(
            InputEvent::getDevice());
    }
};
typedef std::tr1::shared_ptr<MouseEvent> MouseEventPtr;

/** Event called when the cursor moves, and no buttons are down. */
class SIRIKATA_OGRE_EXPORT MouseHoverEvent: public MouseEvent {
public:
    MouseHoverEvent(const PointerDevicePtr &dev,
               float x, float y, int cursorType)
        : MouseEvent(dev, x, y, cursorType) {
    }
};
typedef std::tr1::shared_ptr<MouseHoverEvent> MouseHoverEventPtr;

/** Base class for events involving a mouse click. */
class SIRIKATA_OGRE_EXPORT MouseDownEvent: public MouseEvent {
public:
    MouseButton mButton; ///< The button this event is about.
    float mXStart; ///< X coordinate when the mouse button was first pressed, -1 to 1
    float mYStart; ///< Y coordinate when the mouse button was pressed, -1 to 1
    float mLastX;
    float mLastY;
    int mPressure; ///< Pressure value as defined by SDL.
    int mPressureMax;
    int mPressureMin;

    /** mX - mXStart: Returns a value between -1 and 1 */
    float deltaX() const {
        return (mX - mXStart)/2.0f;
    }

    /** mY - mYStart: Returns a value between -1 and 1 */
    float deltaY() const {
        return (mY - mYStart)/2.0f;
    }

    float deltaLastY() const {
        return (mY - mLastY)/2.0f;
    }

    float deltaLastX() const {
        return (mX - mLastX)/2.0f;
    }

    MouseDownEvent(const PointerDevicePtr &dev,
                    float xstart, float ystart, float xend, float yend,
                    float lastx, float lasty,
                    int cursorType, int button,
                    int pressure, int pressureMin, int pressureMax)
        : MouseEvent(dev, xend, yend, cursorType) {
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
typedef std::tr1::shared_ptr<MouseDownEvent> MouseDownEventPtr;

/** Event when the mouse was clicked (pressed and released
    without moving) */
class SIRIKATA_OGRE_EXPORT MouseClickEvent: public MouseDownEvent {
public:
    MouseClickEvent(const PointerDevicePtr &dev,
                    float x, float y,
                    int cursorType, int button)
        : MouseDownEvent(dev, x, y, x, y, x, y,
                         cursorType, button, 0, 0, 0) {
    }
};
typedef std::tr1::shared_ptr<MouseClickEvent> MouseClickEventPtr;

/** Event when the mouse was pressed. Always sent. */
class SIRIKATA_OGRE_EXPORT MousePressedEvent: public MouseDownEvent {
public:
    MousePressedEvent(const PointerDevicePtr &dev,
                    float x, float y,
                    int cursorType, int button)
        : MouseDownEvent(dev, x, y, x, y, x, y,
                         cursorType, button, 0, 0, 0) {
    }
};
typedef std::tr1::shared_ptr<MousePressedEvent> MousePressedEventPtr;

/** Event when the mouse was released. Always sent. */
class SIRIKATA_OGRE_EXPORT MouseReleasedEvent: public MouseDownEvent {
public:
    MouseReleasedEvent(const PointerDevicePtr &dev,
                    float x, float y,
                    int cursorType, int button)
        : MouseDownEvent(dev, x, y, x, y, x, y,
                         cursorType, button, 0, 0, 0) {
    }
};
typedef std::tr1::shared_ptr<MouseReleasedEvent> MouseReleasedEventPtr;

/** Event when the mouse was dragged. If this event fires, then
    you will not get a MouseClickEvent. */
class SIRIKATA_OGRE_EXPORT MouseDragEvent: public MouseDownEvent {
public:
    MouseDragType mType;
    MouseDragEvent(const PointerDevicePtr &dev,
                   MouseDragType type,
                   float xstart, float ystart, float xend, float yend,
                   float lastx, float lasty,
                   int cursorType, int button,
                   int pressure, int pressureMin, int pressureMax)
        : MouseDownEvent(dev, xstart, ystart,
                         xend, yend, lastx, lasty, cursorType, button,
                         pressure, pressureMin, pressureMax) {
        mType = type;
    }
};
typedef std::tr1::shared_ptr<MouseDragEvent> MouseDragEventPtr;

/** Various SDL-specific events relating to the status of the
    top-level window. */
class SIRIKATA_OGRE_EXPORT WindowEvent {
public:
    enum WindowEventName {
        WindowShown,
        WindowExposed,
        WindowHidden,
        WindowMoved,
        WindowResized,
        WindowMinimized,
        WindowMaximized,
        WindowRestored,
        WindowMouseEnter,
        WindowMouseLeave,
        WindowFocused,
        WindowUnfocused,
        WindowQuit,
        Unknown
    };

    WindowEventName mName;
    int mData1;
    int mData2;

    WindowEvent(WindowEventName name, int data1, int data2)
        : mName(name), mData1(data1), mData2(data2)
    {
    }

    WindowEventName name() const { return mName; }

    int getX() const {
        return mData1;
    }
    int getY() const {
        return mData2;
    }

};
typedef std::tr1::shared_ptr<WindowEvent> WindowEventPtr;

class SIRIKATA_OGRE_EXPORT DragAndDropEvent {
public:
    int mXCoord;
    int mYCoord;
    std::vector<std::string> mFilenames;
    DragAndDropEvent(const std::vector<std::string>&files, int x=0, int y=0)
        : mXCoord(x), mYCoord(y), mFilenames(files) {
    }
};
typedef std::tr1::shared_ptr<DragAndDropEvent> DragAndDropEventPtr;


/** Events generated by a WebView calling Client.event(name, [args]) */
class SIRIKATA_OGRE_EXPORT WebViewEvent : public InputEvent {
public:
    Graphics::WebView* wv;
    String webview;
    String name;
    std::vector<String> args; // The pointer here is annoying, but necessary to avoid having to include the defintion here, which in turn causes circular includes

    WebViewEvent(const String &wvName, const String& name, const std::vector<String>& args);
    WebViewEvent(const String &wvName, const std::vector<DataReference<const char*> >& args);
    virtual ~WebViewEvent();
};
typedef std::tr1::shared_ptr<WebViewEvent> WebViewEventPtr;


}
}

#endif

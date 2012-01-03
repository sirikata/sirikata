// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_OGRE_INPUT_LISTENER_HPP_
#define _SIRIKATA_OGRE_INPUT_LISTENER_HPP_

#include <sirikata/ogre/Platform.hpp>

namespace Sirikata {
namespace Input {

// Event types
class InputDeviceEvent;
typedef std::tr1::shared_ptr<InputDeviceEvent> InputDeviceEventPtr;
class ButtonPressed;
typedef std::tr1::shared_ptr<ButtonPressed> ButtonPressedPtr;
class ButtonRepeated;
typedef std::tr1::shared_ptr<ButtonRepeated> ButtonRepeatedPtr;
class ButtonReleased;
typedef std::tr1::shared_ptr<ButtonReleased> ButtonReleasedPtr;
class ButtonDown;
typedef std::tr1::shared_ptr<ButtonDown> ButtonDownPtr;
class AxisEvent;
typedef std::tr1::shared_ptr<AxisEvent> AxisEventPtr;
class TextInputEvent;
typedef std::tr1::shared_ptr<TextInputEvent> TextInputEventPtr;
class MouseHoverEvent;
typedef std::tr1::shared_ptr<MouseHoverEvent> MouseHoverEventPtr;
class MousePressedEvent;
typedef std::tr1::shared_ptr<MousePressedEvent> MousePressedEventPtr;
class MouseReleasedEvent;
typedef std::tr1::shared_ptr<MouseReleasedEvent> MouseReleasedEventPtr;
class MouseClickEvent;
typedef std::tr1::shared_ptr<MouseClickEvent> MouseClickEventPtr;
class MouseDragEvent;
typedef std::tr1::shared_ptr<MouseDragEvent> MouseDragEventPtr;
class DragAndDropEvent;
typedef std::tr1::shared_ptr<DragAndDropEvent> DragAndDropEventPtr;
class WebViewEvent;
typedef std::tr1::shared_ptr<WebViewEvent> WebViewEventPtr;
class WindowEvent;
typedef std::tr1::shared_ptr<WindowEvent> WindowEventPtr;


/**
 * Defines the set of return values for an EventListener. An acceptable
 * value includes the bitwise or of any values in the enum.
 */
class SIRIKATA_OGRE_EXPORT EventResponse {
    enum {
        NOP,
        DELETE_LISTENER=1,
        CANCEL_EVENT=2,
        DELETE_LISTENER_AND_CANCEL_EVENT = DELETE_LISTENER | CANCEL_EVENT,
    } mResp;

    template <class Ev> friend class EventManager;

  public:
    /// the event listener will be called again, and event stays on the queue.
    static EventResponse nop() {
        EventResponse retval;
        retval.mResp=NOP;
        return retval;
    }
    /// Never call this function again (One-shot listener).
    static EventResponse del() {
        EventResponse retval;
        retval.mResp=DELETE_LISTENER;
        return retval;
    }
    /// Take the event off the queue after this stage (EARLY,MIDDLE,LATE)
    static EventResponse cancel() {
        EventResponse retval;
        retval.mResp=CANCEL_EVENT;
        return retval;
    }
    /// Kill the listener and kill the event (probably not useful).
    static EventResponse cancelAndDel() {
        EventResponse retval;
        retval.mResp=DELETE_LISTENER_AND_CANCEL_EVENT;
        return retval;
    }

    bool operator==(const EventResponse& other) {
        return (mResp == other.mResp);
    }
    bool operator!=(const EventResponse& other) {
        return (mResp != other.mResp);
    }
};

/** InputListener receives events from the InputManager. */
class SIRIKATA_OGRE_EXPORT InputListener {
  public:

    virtual ~InputListener() {}

    virtual EventResponse onInputDeviceEvent(InputDeviceEventPtr ev) { return EventResponse::nop(); }

    virtual EventResponse onKeyPressedEvent(ButtonPressedPtr ev) { return EventResponse::nop(); }
    virtual EventResponse onKeyRepeatedEvent(ButtonRepeatedPtr ev) { return EventResponse::nop(); }
    virtual EventResponse onKeyReleasedEvent(ButtonReleasedPtr ev) { return EventResponse::nop(); }
    virtual EventResponse onKeyDownEvent(ButtonDownPtr ev) { return EventResponse::nop(); }

    virtual EventResponse onAxisEvent(AxisEventPtr ev) { return EventResponse::nop(); }
    virtual EventResponse onTextInputEvent(TextInputEventPtr ev) { return EventResponse::nop(); }
    virtual EventResponse onMouseHoverEvent(MouseHoverEventPtr ev) { return EventResponse::nop(); }
    virtual EventResponse onMousePressedEvent(MousePressedEventPtr ev) { return EventResponse::nop(); }
    virtual EventResponse onMouseReleasedEvent(MouseReleasedEventPtr ev) { return EventResponse::nop(); }
    virtual EventResponse onMouseClickEvent(MouseClickEventPtr ev) { return EventResponse::nop(); }
    virtual EventResponse onMouseDragEvent(MouseDragEventPtr ev) { return EventResponse::nop(); }
    virtual EventResponse onDragAndDropEvent(DragAndDropEventPtr ev) { return EventResponse::nop(); }
    virtual EventResponse onWebViewEvent(WebViewEventPtr ev) { return EventResponse::nop(); }
    virtual EventResponse onWindowEvent(WindowEventPtr ev) { return EventResponse::nop(); }

}; // class InputListener

} // namespace Input
} // namespace Sirikata

#endif //_SIRIKATA_OGRE_INPUT_LISTENER_HPP_

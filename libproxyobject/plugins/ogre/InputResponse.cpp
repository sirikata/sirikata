/*  Sirikata libproxyobject -- Ogre Graphics Plugin
 *  InputResponse.cpp
 *
 *  Copyright (c) 2009, Ewen Cheslack-Postava
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

#include <boost/lexical_cast.hpp>
#include "InputResponse.hpp"

namespace Sirikata {
namespace Graphics {

using namespace Input;

InputResponse::~InputResponse() {
}

void InputResponse::invoke(ButtonEventPtr& ev) {
    ButtonPressedEventPtr button_pressed_ev (std::tr1::dynamic_pointer_cast<ButtonPressed>(ev));
    if (button_pressed_ev) {
        invoke(button_pressed_ev);
        return;
    }

    ButtonReleasedEventPtr button_released_ev (std::tr1::dynamic_pointer_cast<ButtonReleased>(ev));
    if (button_released_ev) {
        invoke(button_released_ev);
        return;
    }

    assert(false);
}

void InputResponse::invoke(ButtonPressedEventPtr& evt) {
    defaultAction();
}

void InputResponse::invoke(ButtonReleasedEventPtr& evt) {
    defaultAction();
}

void InputResponse::invoke(ButtonDownEventPtr& evt) {
    defaultAction();
}

void InputResponse::invoke(AxisEventPtr& evt) {
    defaultAction();
}

void InputResponse::invoke(TextInputEventPtr& evt) {
    defaultAction();
}

void InputResponse::invoke(MouseHoverEventPtr& evt) {
    defaultAction();
}

void InputResponse::invoke(MouseClickEventPtr& evt) {
    defaultAction();
}

void InputResponse::invoke(MouseDragEventPtr& evt) {
    defaultAction();
}

void InputResponse::invoke(WindowEventPtr& evt) {
    defaultAction();
}

void InputResponse::invoke(DragAndDropEventPtr& evt) {
    defaultAction();
}

void InputResponse::invoke(WebViewEventPtr& evt) {
    defaultAction();
}

void InputResponse::invoke(InputEventPtr& ev) {
    ButtonPressedEventPtr button_pressed_ev (std::tr1::dynamic_pointer_cast<ButtonPressed>(ev));
    if (button_pressed_ev) {
        invoke(button_pressed_ev);
        return;
    }

    ButtonReleasedEventPtr button_released_ev (std::tr1::dynamic_pointer_cast<ButtonReleased>(ev));
    if (button_released_ev) {
        invoke(button_released_ev);
        return;
    }

    ButtonDownEventPtr button_down_ev (std::tr1::dynamic_pointer_cast<ButtonDown>(ev));
    if (button_down_ev) {
        invoke(button_down_ev);
        return;
    }

    AxisEventPtr axis_ev (std::tr1::dynamic_pointer_cast<AxisEvent>(ev));
    if (axis_ev) {
        invoke(axis_ev);
        return;
    }

    TextInputEventPtr text_input_ev (std::tr1::dynamic_pointer_cast<TextInputEvent>(ev));
    if (text_input_ev) {
        invoke(text_input_ev);
        return;
    }

    MouseHoverEventPtr mouse_hover_ev (std::tr1::dynamic_pointer_cast<MouseHoverEvent>(ev));
    if (mouse_hover_ev) {
        invoke(mouse_hover_ev);
        return;
    }

    MouseClickEventPtr mouse_click_ev (std::tr1::dynamic_pointer_cast<MouseClickEvent>(ev));
    if (mouse_click_ev) {
        invoke(mouse_click_ev);
        return;
    }

    MouseDragEventPtr mouse_drag_ev (std::tr1::dynamic_pointer_cast<MouseDragEvent>(ev));
    if (mouse_drag_ev) {
        invoke(mouse_drag_ev);
        return;
    }

    WindowEventPtr window_ev (std::tr1::dynamic_pointer_cast<WindowEvent>(ev));
    if (window_ev) {
        invoke(window_ev);
        return;
    }

    DragAndDropEventPtr dd_ev (std::tr1::dynamic_pointer_cast<DragAndDropEvent>(ev));
    if (dd_ev) {
        invoke(dd_ev);
        return;
    }

    WebViewEventPtr wv_ev (std::tr1::dynamic_pointer_cast<WebViewEvent>(ev));
    if (wv_ev) {
        invoke(wv_ev);
        return;
    }
}

void InputResponse::defaultAction() {
    // By default, do nothing
}


SimpleInputResponse::SimpleInputResponse(ResponseCallback cb)
 : mCallback(cb)
{
}

void SimpleInputResponse::defaultAction() {
    mCallback();
}

InputResponse::InputEventDescriptorList SimpleInputResponse::getInputEvents(const InputBindingEvent& descriptor) const {
    InputEventDescriptorList result;

    if (descriptor.isKey()) {
        result.push_back(Input::EventDescriptor::Key(descriptor.keyButton(), Input::KEY_PRESSED, descriptor.keyModifiers()));
    }
    else if (descriptor.isWeb()) {
        result.push_back(Input::EventDescriptor::Web(descriptor.webViewName(), descriptor.webName(), descriptor.webArgCount()));
    }

    return result;
}



FloatToggleInputResponse::FloatToggleInputResponse(ResponseCallback cb, float onval, float offval)
 : mCallback(cb),
   mOnValue(onval),
   mOffValue(offval)
{
}

void FloatToggleInputResponse::invoke(Input::ButtonPressedEventPtr& evt) {
    mCallback(mOnValue);
}

void FloatToggleInputResponse::invoke(Input::ButtonReleasedEventPtr& evt) {
    mCallback(mOffValue);
}

void FloatToggleInputResponse::invoke(Input::WebViewEventPtr& evt) {
#ifdef HAVE_BERKELIUM
    if (evt->args.size() > 0) {
        float arg = 0;
        try {
            arg = boost::lexical_cast<float>(evt->args[0]);
        } catch (const boost::bad_lexical_cast &e) {
            SILOG(input,warning,"FloatInputResponse::invoke(WebViewEventPtr) requires float arg");
        }
        mCallback(arg);
    } else {
        SILOG(input,warning,"FloatInputResponse::invoke(WebViewEventPtr) requires >= 1 arg");
    }
#endif
}

InputResponse::InputEventDescriptorList FloatToggleInputResponse::getInputEvents(const InputBindingEvent& descriptor) const {
    InputEventDescriptorList result;

    if (descriptor.isKey()) {
        result.push_back(Input::EventDescriptor::Key(descriptor.keyButton(), Input::KEY_PRESSED, descriptor.keyModifiers()));
        result.push_back(Input::EventDescriptor::Key(descriptor.keyButton(), Input::KEY_RELEASED, descriptor.keyModifiers()));
    }
    if (descriptor.isWeb()) {
        result.push_back(Input::EventDescriptor::Web(descriptor.webViewName(), descriptor.webName(), 1));
    }

    return result;
}



Vector2fInputResponse::Vector2fInputResponse(ResponseCallback cb)
 : mCallback(cb)
{
}

void Vector2fInputResponse::invoke(Input::MouseClickEventPtr& evt) {
    mCallback(Vector2f(evt->mX, evt->mY));
}

void Vector2fInputResponse::invoke(Input::MouseDragEventPtr& evt) {
    mCallback(Vector2f(evt->mX, evt->mY));
}

InputResponse::InputEventDescriptorList Vector2fInputResponse::getInputEvents(const InputBindingEvent& descriptor) const {
    InputEventDescriptorList result;

    if (descriptor.isMouseClick()) {
        result.push_back(Input::EventDescriptor::MouseClick(descriptor.mouseClickButton()));
    }

    return result;
}



AxisInputResponse::AxisInputResponse(ResponseCallback cb)
 : mCallback(cb)
{
}

void AxisInputResponse::invoke(Input::AxisEventPtr& evt) {
    float val = evt->mValue.getCentered();
    InputDevicePtr dev = evt->getDevice();
    if (dev) {
        Vector2f axes(
            dev->getAxis(Input::AXIS_CURSORX).getCentered(),
            dev->getAxis(Input::AXIS_CURSORY).getCentered()
        );
        mCallback(val, axes);
    }
}

InputResponse::InputEventDescriptorList AxisInputResponse::getInputEvents(const InputBindingEvent& descriptor) const {
    InputEventDescriptorList result;

    if (descriptor.isAxis()) {
        result.push_back(Input::EventDescriptor::Axis(descriptor.axisIndex()));
    }

    return result;
}


StringInputResponse::StringInputResponse(ResponseCallback cb)
 : mCallback(cb)
{
}

void StringInputResponse::invoke(WebViewEventPtr& wvevt) {
    const std::vector<String>& args = wvevt->args;
    if (args.size() > 0) {
        mCallback(args[0]);
    } else {
        SILOG(input,fatal,"Empty args in StringInputResponse::invoke(WebViewEventPtr)");
    }
}

InputResponse::InputEventDescriptorList StringInputResponse::getInputEvents(const InputBindingEvent& descriptor) const {
    InputEventDescriptorList result;

    if (descriptor.isWeb()) {
        result.push_back(Input::EventDescriptor::Web(descriptor.webViewName(), descriptor.webName(), descriptor.webArgCount()));
    }

    return result;
}

} // namespace Graphics
} // namespace Sirikata

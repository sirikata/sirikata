/*  Sirikata liboh -- Ogre Graphics Plugin
 *  InputBinding.cpp
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

#include "InputBinding.hpp"

namespace Sirikata {
namespace Graphics {

using namespace Input;

InputBinding::InputBinding() {
}

InputBinding::~InputBinding() {
}

void InputBinding::add(const InputBindingEvent& evt, InputResponse* response) {
    InputResponse::InputEventDescriptorList evts = response->getInputEvents(evt);

    for(InputResponse::InputEventDescriptorList::iterator it = evts.begin(); it != evts.end(); it++)
        mResponses[*it] = response;
}

void InputBinding::handle(Input::ButtonEventPtr& evt) {
    ButtonPressedEventPtr pressed_ev = std::tr1::dynamic_pointer_cast<ButtonPressed>(evt);
    if (pressed_ev) {
        this->handle(pressed_ev);
        return;
    }

    ButtonReleasedEventPtr released_ev = std::tr1::dynamic_pointer_cast<ButtonReleased>(evt);
    if (released_ev) {
        this->handle(released_ev);
        return;
    }

    ButtonDownEventPtr down_ev = std::tr1::dynamic_pointer_cast<ButtonDown>(evt);
    if (down_ev) {
        this->handle(down_ev);
        return;
    }
}

void InputBinding::handle(Input::ButtonPressedEventPtr& evt) {
    Input::EventDescriptor::KeyEventModifier mods = Input::EventDescriptor::KeyModNone;
    switch(evt->mModifier) {
      case InputDevice::MOD_ALT:
        mods = Input::EventDescriptor::KeyModAlt;
        break;
      case InputDevice::MOD_CTRL:
        mods = Input::EventDescriptor::KeyModCtrl;
        break;
      case InputDevice::MOD_SHIFT:
        mods = Input::EventDescriptor::KeyModShift;
        break;
    }

    Input::EventDescriptor descriptor =
        Input::EventDescriptor::Key(evt->mButton, Input::EventDescriptor::KeyPressed, mods);

    Binding::iterator it = mResponses.find(descriptor);
    if (it != mResponses.end()) {
        InputResponse* response = it->second;
        response->invoke(evt);
    }
}

void InputBinding::handle(Input::ButtonReleasedEventPtr& evt) {
    Input::EventDescriptor::KeyEventModifier mods = Input::EventDescriptor::KeyModNone;
    switch(evt->mModifier) {
      case InputDevice::MOD_ALT:
        mods = Input::EventDescriptor::KeyModAlt;
        break;
      case InputDevice::MOD_CTRL:
        mods = Input::EventDescriptor::KeyModCtrl;
        break;
      case InputDevice::MOD_SHIFT:
        mods = Input::EventDescriptor::KeyModShift;
        break;
    }

    Input::EventDescriptor descriptor =
        Input::EventDescriptor::Key(evt->mButton, Input::EventDescriptor::KeyReleased, mods);

    Binding::iterator it = mResponses.find(descriptor);
    if (it != mResponses.end()) {
        InputResponse* response = it->second;
        response->invoke(evt);
    }
}

void InputBinding::handle(Input::ButtonDownEventPtr& evt) {
    assert(false);
}

void InputBinding::handle(Input::AxisEventPtr& evt) {
    Input::EventDescriptor descriptor =
        Input::EventDescriptor::Axis(evt->mAxis);

    Binding::iterator it = mResponses.find(descriptor);
    if (it != mResponses.end()) {
        InputResponse* response = it->second;
        response->invoke(evt);
    }
}

void InputBinding::handle(Input::TextInputEventPtr& evt) {
    assert(false);
}

void InputBinding::handle(Input::MouseHoverEventPtr& evt) {
    assert(false);
}

void InputBinding::handle(Input::MouseClickEventPtr& evt) {
    Input::EventDescriptor descriptor =
        Input::EventDescriptor::MouseClick(evt->mButton);

    Binding::iterator it = mResponses.find(descriptor);
    if (it != mResponses.end()) {
        InputResponse* response = it->second;
        response->invoke(evt);
    }
}

void InputBinding::handle(Input::MouseDragEventPtr& evt) {
    assert(false);
}

void InputBinding::handle(Input::WindowEventPtr& evt) {
    assert(false);
}

void InputBinding::handle(Input::DragAndDropEventPtr& evt) {
    assert(false);
}

} // namespace Graphics
} // namespace Sirikata

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
namespace SimpleCamera {

InputResponse::~InputResponse() {
}

void InputResponse::invoke(InputBindingEvent& evt) {
    defaultAction();
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
        result.push_back(InputBindingEvent::Key(descriptor.keyButton(), descriptor.keyModifiers()));
    }
    else if (descriptor.isWeb()) {
        result.push_back(InputBindingEvent::Web(descriptor.webViewName(), descriptor.webName()));
    }

    return result;
}



FloatToggleInputResponse::FloatToggleInputResponse(ResponseCallback cb, float onval, float offval)
 : mCallback(cb),
   mOnValue(onval),
   mOffValue(offval)
{
}

void FloatToggleInputResponse::invoke(InputBindingEvent& evt) {
    if (evt.isKey()) {
        if (evt.keyPressed())
            mCallback(mOnValue);
        else if (evt.keyReleased())
            mCallback(mOffValue);
    }
}


InputResponse::InputEventDescriptorList FloatToggleInputResponse::getInputEvents(const InputBindingEvent& descriptor) const {
    InputEventDescriptorList result;

    if (descriptor.isKey()) {
        result.push_back(InputBindingEvent::Key(descriptor.keyButton(), descriptor.keyModifiers()));
        result.push_back(InputBindingEvent::Key(descriptor.keyButton(), descriptor.keyModifiers()));
    }
    if (descriptor.isWeb()) {
        result.push_back(InputBindingEvent::Web(descriptor.webViewName(), descriptor.webName()));
    }

    return result;
}



Vector2fInputResponse::Vector2fInputResponse(ResponseCallback cb)
 : mCallback(cb)
{
}

void Vector2fInputResponse::invoke(InputBindingEvent& evt) {
    if (evt.isMouseClick())
        mCallback(Vector2f(evt.mouseX(), evt.mouseY()));
    else if (evt.isMouseDrag())
        mCallback(Vector2f(evt.mouseX(), evt.mouseY()));
}

InputResponse::InputEventDescriptorList Vector2fInputResponse::getInputEvents(const InputBindingEvent& descriptor) const {
    InputEventDescriptorList result;

    if (descriptor.isMouseClick()) {
        result.push_back(InputBindingEvent::MouseClick(descriptor.mouseClickButton()));
    }

    return result;
}



AxisInputResponse::AxisInputResponse(ResponseCallback cb)
 : mCallback(cb)
{
}

void AxisInputResponse::invoke(InputBindingEvent& evt) {
    if (evt.isAxis()) {
        float val = evt.axisValue();
        mCallback(val);
    }
}

InputResponse::InputEventDescriptorList AxisInputResponse::getInputEvents(const InputBindingEvent& descriptor) const {
    InputEventDescriptorList result;

    if (descriptor.isAxis()) {
        result.push_back(InputBindingEvent::Axis(descriptor.axisIndex()));
    }

    return result;
}

} // namespace SimpleCamera
} // namespace Sirikata

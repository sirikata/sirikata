/*  Sirikata libproxyobject -- Ogre Graphics Plugin
 *  InputBindingEvent.cpp
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

#include "InputBindingEvent.hpp"

// Keycodes are SDL specific, not sure why we have the InputDevice abstraction
// since we can't and don't currently take advantage of it in any way
#include "input/SDLInputDevice.hpp"

namespace Sirikata {
namespace Graphics {

static void init_button_conversion_maps();

using namespace Input;

InputBindingEvent InputBindingEvent::Key(KeyButton button, Modifier mod) {
    InputBindingEvent result;
    result.mTag = KeyEventTag;
    result.mDescriptor.key.button = button;
    result.mDescriptor.key.mod = mod;
    return result;
}

InputBindingEvent InputBindingEvent::MouseClick(MouseButton button) {
    InputBindingEvent result;
    result.mTag = MouseClickEventTag;
    result.mDescriptor.mouseClick.button = button;
    return result;
}

InputBindingEvent InputBindingEvent::MouseDrag(MouseButton button) {
    InputBindingEvent result;
    result.mTag = MouseDragEventTag;
    result.mDescriptor.mouseDrag.button = button;
    return result;
}

InputBindingEvent InputBindingEvent::Axis(AxisIndex index) {
    InputBindingEvent result;
    result.mTag = AxisEventTag;
    result.mDescriptor.axis.index = index;
    return result;
}

InputBindingEvent InputBindingEvent::Web(const String& wvname, const String& name) {
    InputBindingEvent result;
    result.mTag = WebEventTag;
    result.mDescriptor.web.wvname = new String(wvname);
    result.mDescriptor.web.name = new String(name);
    return result;
}

InputBindingEvent::InputBindingEvent()
 : mTag(Bogus)
{
}

InputBindingEvent::InputBindingEvent(const InputBindingEvent& other) {
    *this = other;
}

InputBindingEvent::~InputBindingEvent() {
    if (isWeb()) {
        delete mDescriptor.web.wvname;
        mDescriptor.web.wvname = NULL;
        delete mDescriptor.web.name;
        mDescriptor.web.name = NULL;
    }
}


bool InputBindingEvent::valid() const {
    return mTag != Bogus;
}

bool InputBindingEvent::isKey() const {
    return mTag == KeyEventTag;
}

KeyButton InputBindingEvent::keyButton() const {
    assert(isKey());
    return mDescriptor.key.button;
}

Modifier InputBindingEvent::keyModifiers() const {
    assert(isKey());
    return mDescriptor.key.mod;
}


bool InputBindingEvent::isMouseClick() const {
    return mTag == MouseClickEventTag;
}

MouseButton InputBindingEvent::mouseClickButton() const {
    assert(isMouseClick());
    return mDescriptor.mouseClick.button;
}


bool InputBindingEvent::isMouseDrag() const {
    return mTag == MouseDragEventTag;
}

MouseButton InputBindingEvent::mouseDragButton() const {
    assert(isMouseDrag());
    return mDescriptor.mouseDrag.button;
}


bool InputBindingEvent::isAxis() const {
    return mTag == AxisEventTag;
}

AxisIndex InputBindingEvent::axisIndex() const {
    assert(isAxis());
    return mDescriptor.axis.index;
}

bool InputBindingEvent::isWeb() const {
    return mTag == WebEventTag;
}

const String& InputBindingEvent::webViewName() const {
    assert(isWeb());
    return *mDescriptor.web.wvname;
}

const String& InputBindingEvent::webName() const {
    assert(isWeb());
    return *mDescriptor.web.name;
}



InputBindingEvent& InputBindingEvent::operator=(const InputBindingEvent& rhs) {
    mTag = rhs.mTag;
    mDescriptor = rhs.mDescriptor;

    // We need to make a copy of the strings
    if (isWeb()) {
        mDescriptor.web.wvname = new String(*rhs.mDescriptor.web.wvname);
        mDescriptor.web.name = new String(*rhs.mDescriptor.web.name);
    }

    return *this;
}

String InputBindingEvent::toString() const {
    switch(mTag) {
      case KeyEventTag:
          {
              String result = String("key-");
              String mods = keyModifiersString(keyModifiers());
              if (!mods.empty()) result += (mods + "-");
              result += keyButtonString(keyButton());
              return result;
          }
        break;
      case MouseHoverEventTag:
        return "hover-";
        break;
      case MousePressedEventTag:
        return "pressed-";
        break;
      case MouseClickEventTag:
        return "click-" + mouseButtonString(mouseClickButton());
        break;
      case MouseDragEventTag:
        return "drag-" + mouseButtonString(mouseDragButton());
        break;
      case AxisEventTag:
        return "axis-" + axisString(axisIndex());
        break;
      case TextEventTag:
        return "text-";
        break;
      case WindowEventTag:
        return "";
        break;
      case DragAndDropEventTag:
        return "";
        break;
      case WebEventTag:
        return (String("web-") + webViewName() + String("-") + webName());
        break;
      default:
        SILOG(ogre, error, "[OGRE] Unhandled input event type: " << (int32)mTag);
        return "";
    }
}

InputBindingEvent InputBindingEvent::fromString(const String& asString) {
    // First, split into sections by -'s
    std::vector<String> parts;
    int32 last_idx = -1, next_idx = 0;
    while(next_idx != String::npos) {
        next_idx = asString.find('-', last_idx+1);
        String part = asString.substr(last_idx+1, (next_idx == String::npos ? String::npos : next_idx-last_idx-1));
        parts.push_back(part);
        last_idx = next_idx;
    }

    if (parts.empty()) return InputBindingEvent();
    String major = *parts.begin();
    parts.erase(parts.begin());
    if (major == "key") {
        Input::Modifier mod = keyModifiersFromStrings(parts);
        Input::KeyButton but = keyButtonFromStrings(parts);
        return InputBindingEvent::Key(but, mod);
    }
    else if (major == "click") {
        Input::MouseButton but = mouseButtonFromStrings(parts);
        return InputBindingEvent::MouseClick(but);
    }
    else if (major == "drag") {
        Input::MouseButton but = mouseButtonFromStrings(parts);
        return InputBindingEvent::MouseDrag(but);
    }
    else if (major == "axis") {
        Input::AxisIndex idx = axisFromStrings(parts);
        return InputBindingEvent::Axis(idx);
    }
    else if (major == "web") {
        assert(parts.size() == 2);
        return InputBindingEvent::Web(parts[0], parts[1]);
    }
    else {
        return InputBindingEvent();
    }
}


std::ostream& operator<<(std::ostream& os, const Sirikata::Graphics::InputBindingEvent& ibe) {
    os << ibe.toString();
    return os;
}

std::istream& operator>>(std::istream& is, Sirikata::Graphics::InputBindingEvent& ibe) {
    // We're assuming that these are separated as strings are. We could use a
    // more complicated approach that takes in more -xyz- sections and tries to
    // parse them to allow these to be used inline with other items, but we'd
    // need some sort of delimiter or two of these next to each other wouldn't
    // parse.
    Sirikata::String internal;
    is >> internal;
    ibe = Sirikata::Graphics::InputBindingEvent::fromString(internal);
    return is;
}

} // namespace Graphics
} // namespace Sirikata

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
#include <boost/lexical_cast.hpp>
#include <SDL_keysym.h>

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

void ensure_initialized() {
    static bool initialized = false;
    if (!initialized) {
        initialized = true;
        init_button_conversion_maps();
    }
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

static std::map<Input::KeyButton, String> ScancodesToStrings;
static std::map<String, Input::KeyButton> StringsToScancodes;

#define INIT_SCANCODE_STRING_MAP(X, STR)                     \
    ScancodesToStrings[SDL_SCANCODE_##X] = #STR;             \
    StringsToScancodes[#STR] = SDL_SCANCODE_##X

static void init_button_conversion_maps() {
    INIT_SCANCODE_STRING_MAP(A, a); INIT_SCANCODE_STRING_MAP(B, b); INIT_SCANCODE_STRING_MAP(C, c);
    INIT_SCANCODE_STRING_MAP(D, d); INIT_SCANCODE_STRING_MAP(E, e); INIT_SCANCODE_STRING_MAP(F, f);
    INIT_SCANCODE_STRING_MAP(G, g); INIT_SCANCODE_STRING_MAP(H, h); INIT_SCANCODE_STRING_MAP(I, i);
    INIT_SCANCODE_STRING_MAP(J, j); INIT_SCANCODE_STRING_MAP(K, k); INIT_SCANCODE_STRING_MAP(L, l);
    INIT_SCANCODE_STRING_MAP(M, m); INIT_SCANCODE_STRING_MAP(N, n); INIT_SCANCODE_STRING_MAP(O, o);
    INIT_SCANCODE_STRING_MAP(P, p); INIT_SCANCODE_STRING_MAP(Q, q); INIT_SCANCODE_STRING_MAP(R, r);
    INIT_SCANCODE_STRING_MAP(S, s); INIT_SCANCODE_STRING_MAP(T, t); INIT_SCANCODE_STRING_MAP(U, u);
    INIT_SCANCODE_STRING_MAP(V, v); INIT_SCANCODE_STRING_MAP(W, w); INIT_SCANCODE_STRING_MAP(X, X);
    INIT_SCANCODE_STRING_MAP(Y, y); INIT_SCANCODE_STRING_MAP(Z, z); INIT_SCANCODE_STRING_MAP(0, 0);
    INIT_SCANCODE_STRING_MAP(1, 1); INIT_SCANCODE_STRING_MAP(2, 2); INIT_SCANCODE_STRING_MAP(3, 3);
    INIT_SCANCODE_STRING_MAP(4, 4); INIT_SCANCODE_STRING_MAP(5, 5); INIT_SCANCODE_STRING_MAP(6, 6);
    INIT_SCANCODE_STRING_MAP(7, 7); INIT_SCANCODE_STRING_MAP(8, 8); INIT_SCANCODE_STRING_MAP(9, 9);

    INIT_SCANCODE_STRING_MAP(LSHIFT, lshift);
    INIT_SCANCODE_STRING_MAP(RSHIFT, rshift);
    INIT_SCANCODE_STRING_MAP(LCTRL, lctrl);
    INIT_SCANCODE_STRING_MAP(RCTRL, rctrl);
    INIT_SCANCODE_STRING_MAP(LALT, lalt);
    INIT_SCANCODE_STRING_MAP(RALT, ralt);
    INIT_SCANCODE_STRING_MAP(LGUI, lsuper);
    INIT_SCANCODE_STRING_MAP(RGUI, rsuper);
    INIT_SCANCODE_STRING_MAP(RETURN, return);
    INIT_SCANCODE_STRING_MAP(ESCAPE, escape);
    INIT_SCANCODE_STRING_MAP(BACKSPACE, back);
    INIT_SCANCODE_STRING_MAP(TAB, tab);
    INIT_SCANCODE_STRING_MAP(SPACE, space);
    INIT_SCANCODE_STRING_MAP(MINUS, minus);
    INIT_SCANCODE_STRING_MAP(EQUALS, equals);
    INIT_SCANCODE_STRING_MAP(LEFTBRACKET, [);
    INIT_SCANCODE_STRING_MAP(RIGHTBRACKET, ]);
    INIT_SCANCODE_STRING_MAP(BACKSLASH, backslash);
    INIT_SCANCODE_STRING_MAP(SEMICOLON, semicolon);
    INIT_SCANCODE_STRING_MAP(APOSTROPHE, apostrophe);
    INIT_SCANCODE_STRING_MAP(GRAVE, OEM_3);
    INIT_SCANCODE_STRING_MAP(COMMA, comma);
    INIT_SCANCODE_STRING_MAP(PERIOD, period);
    INIT_SCANCODE_STRING_MAP(SLASH, OEM_2);
    INIT_SCANCODE_STRING_MAP(CAPSLOCK, CAPITAL);
    INIT_SCANCODE_STRING_MAP(F1, F1);
    INIT_SCANCODE_STRING_MAP(F2, F2);
    INIT_SCANCODE_STRING_MAP(F3, F3);
    INIT_SCANCODE_STRING_MAP(F4, F4);
    INIT_SCANCODE_STRING_MAP(F5, F5);
    INIT_SCANCODE_STRING_MAP(F6, F6);
    INIT_SCANCODE_STRING_MAP(F7, F7);
    INIT_SCANCODE_STRING_MAP(F8, F8);
    INIT_SCANCODE_STRING_MAP(F9, F9);
    INIT_SCANCODE_STRING_MAP(F10, F10);
    INIT_SCANCODE_STRING_MAP(F11, F11);
    INIT_SCANCODE_STRING_MAP(F12, F12);
    INIT_SCANCODE_STRING_MAP(PRINTSCREEN, print);
    INIT_SCANCODE_STRING_MAP(SCROLLLOCK, scroll);
    INIT_SCANCODE_STRING_MAP(PAUSE, pause);
    INIT_SCANCODE_STRING_MAP(INSERT, insert);
    INIT_SCANCODE_STRING_MAP(HOME, home);
    INIT_SCANCODE_STRING_MAP(PAGEUP, pageup);
    INIT_SCANCODE_STRING_MAP(DELETE, delete);
    INIT_SCANCODE_STRING_MAP(END, end);
    INIT_SCANCODE_STRING_MAP(PAGEDOWN, pagedown);
    INIT_SCANCODE_STRING_MAP(RIGHT, right);
    INIT_SCANCODE_STRING_MAP(LEFT, left);
    INIT_SCANCODE_STRING_MAP(DOWN, down);
    INIT_SCANCODE_STRING_MAP(UP, up);
    INIT_SCANCODE_STRING_MAP(KP_0, insert);
    INIT_SCANCODE_STRING_MAP(KP_1, end);
    INIT_SCANCODE_STRING_MAP(KP_2, kpdown);
    INIT_SCANCODE_STRING_MAP(KP_3, next);
    INIT_SCANCODE_STRING_MAP(KP_4, kpleft);
    INIT_SCANCODE_STRING_MAP(KP_6, kpright);
    INIT_SCANCODE_STRING_MAP(KP_7, home);
    INIT_SCANCODE_STRING_MAP(KP_8, kpup);
    INIT_SCANCODE_STRING_MAP(KP_9, prior);
}


String InputBindingEvent::keyButtonString(Input::KeyButton b) const {
    ensure_initialized();
    if (ScancodesToStrings.find(b) != ScancodesToStrings.end())
        return ScancodesToStrings[b];
    return "";
}

String InputBindingEvent::keyModifiersString(Input::Modifier m) const {
    if (m == Input::MOD_NONE)
        return "";
    String result;
    if (m & MOD_SHIFT) {
        result += "shift";
    }
    if (m & MOD_CTRL) {
        if (!result.empty()) result += "-";
        result += "ctrl";
    }
    if (m & MOD_ALT) {
        if (!result.empty()) result += "-";
        result += "alt";
    }
    if (m & MOD_GUI) {
        if (!result.empty()) result += "-";
        result += "super";
    }
    return result;
}

String InputBindingEvent::mouseButtonString(Input::MouseButton b) const {
    return boost::lexical_cast<String>(b);
}

String InputBindingEvent::axisString(Input::AxisIndex i) const {
    return boost::lexical_cast<String>(i);
}

Input::KeyButton InputBindingEvent::keyButtonFromStrings(std::vector<String>& parts) {
    ensure_initialized();

    assert(!parts.empty());
    const String& part = *parts.begin();
    Input::KeyButton result = 0;
    if (StringsToScancodes.find(part) != StringsToScancodes.end()) {
        result = StringsToScancodes[part];
        parts.erase(parts.begin());
    }
    return result;
}

Input::Modifier InputBindingEvent::keyModifiersFromStrings(std::vector<String>& parts) {
    bool more = true;
    Input::Modifier result = Input::MOD_NONE;
    while(more && !parts.empty()) {
        const String& val = *parts.begin();
        if (val == "shift")
            result |= Input::MOD_SHIFT;
        else if (val == "ctrl")
            result |= Input::MOD_CTRL;
        else if (val == "alt")
            result |= Input::MOD_ALT;
        else if (val == "super")
            result |= Input::MOD_GUI;
        else
            more = false;
        if (more) // We got something, so we need to remove it
            parts.erase(parts.begin());
    }
    return result;
}

Input::MouseButton InputBindingEvent::mouseButtonFromStrings(std::vector<String>& parts) {
    assert(!parts.empty());
    Input::MouseButton result = boost::lexical_cast<Input::MouseButton>(*parts.begin());
    parts.erase( parts.begin() );
    return result;
}

Input::AxisIndex InputBindingEvent::axisFromStrings(std::vector<String>& parts) {
    assert(!parts.empty());
    Input::AxisIndex result = boost::lexical_cast<Input::AxisIndex>(*parts.begin());
    parts.erase( parts.begin() );
    return result;
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

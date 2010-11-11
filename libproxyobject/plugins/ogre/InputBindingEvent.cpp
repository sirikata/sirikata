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


#define MAP_SCANCODE_TO_STR(X, STR) case SDL_SCANCODE_##X: return #STR

String InputBindingEvent::keyButtonString(Input::KeyButton b) const {
    switch(b)
    {
        MAP_SCANCODE_TO_STR(A, a); MAP_SCANCODE_TO_STR(B, b); MAP_SCANCODE_TO_STR(C, c);
        MAP_SCANCODE_TO_STR(D, d); MAP_SCANCODE_TO_STR(E, e); MAP_SCANCODE_TO_STR(F, f);
        MAP_SCANCODE_TO_STR(G, g); MAP_SCANCODE_TO_STR(H, h); MAP_SCANCODE_TO_STR(I, i);
        MAP_SCANCODE_TO_STR(J, j); MAP_SCANCODE_TO_STR(K, k); MAP_SCANCODE_TO_STR(L, l);
        MAP_SCANCODE_TO_STR(M, m); MAP_SCANCODE_TO_STR(N, n); MAP_SCANCODE_TO_STR(O, o);
        MAP_SCANCODE_TO_STR(P, p); MAP_SCANCODE_TO_STR(Q, q); MAP_SCANCODE_TO_STR(R, r);
        MAP_SCANCODE_TO_STR(S, s); MAP_SCANCODE_TO_STR(T, t); MAP_SCANCODE_TO_STR(U, u);
        MAP_SCANCODE_TO_STR(V, v); MAP_SCANCODE_TO_STR(W, w); MAP_SCANCODE_TO_STR(X, X);
        MAP_SCANCODE_TO_STR(Y, y); MAP_SCANCODE_TO_STR(Z, z); MAP_SCANCODE_TO_STR(0, 0);
        MAP_SCANCODE_TO_STR(1, 1); MAP_SCANCODE_TO_STR(2, 2); MAP_SCANCODE_TO_STR(3, 3);
        MAP_SCANCODE_TO_STR(4, 4); MAP_SCANCODE_TO_STR(5, 5); MAP_SCANCODE_TO_STR(6, 6);
        MAP_SCANCODE_TO_STR(7, 7); MAP_SCANCODE_TO_STR(8, 8); MAP_SCANCODE_TO_STR(9, 9);

        MAP_SCANCODE_TO_STR(LSHIFT, lshift);
        MAP_SCANCODE_TO_STR(RSHIFT, rshift);
        MAP_SCANCODE_TO_STR(LCTRL, lctrl);
        MAP_SCANCODE_TO_STR(RCTRL, rctrl);
        MAP_SCANCODE_TO_STR(LALT, lalt);
        MAP_SCANCODE_TO_STR(RALT, ralt);
        MAP_SCANCODE_TO_STR(LGUI, lsuper);
        MAP_SCANCODE_TO_STR(RGUI, rsuper);
        MAP_SCANCODE_TO_STR(RETURN, return);
        MAP_SCANCODE_TO_STR(ESCAPE, escape);
        MAP_SCANCODE_TO_STR(BACKSPACE, back);
        MAP_SCANCODE_TO_STR(TAB, tab);
        MAP_SCANCODE_TO_STR(SPACE, space);
        MAP_SCANCODE_TO_STR(MINUS, minus);
        MAP_SCANCODE_TO_STR(EQUALS, equals);
        MAP_SCANCODE_TO_STR(LEFTBRACKET, [);
        MAP_SCANCODE_TO_STR(RIGHTBRACKET, ]);
        MAP_SCANCODE_TO_STR(BACKSLASH, backslash);
        MAP_SCANCODE_TO_STR(SEMICOLON, semicolon);
        MAP_SCANCODE_TO_STR(APOSTROPHE, apostrophe);
        MAP_SCANCODE_TO_STR(GRAVE, OEM_3);
        MAP_SCANCODE_TO_STR(COMMA, comma);
        MAP_SCANCODE_TO_STR(PERIOD, period);
        MAP_SCANCODE_TO_STR(SLASH, OEM_2);
        MAP_SCANCODE_TO_STR(CAPSLOCK, CAPITAL);
        MAP_SCANCODE_TO_STR(F1, F1);
        MAP_SCANCODE_TO_STR(F2, F2);
        MAP_SCANCODE_TO_STR(F3, F3);
        MAP_SCANCODE_TO_STR(F4, F4);
        MAP_SCANCODE_TO_STR(F5, F5);
        MAP_SCANCODE_TO_STR(F6, F6);
        MAP_SCANCODE_TO_STR(F7, F7);
        MAP_SCANCODE_TO_STR(F8, F8);
        MAP_SCANCODE_TO_STR(F9, F9);
        MAP_SCANCODE_TO_STR(F10, F10);
        MAP_SCANCODE_TO_STR(F11, F11);
        MAP_SCANCODE_TO_STR(F12, F12);
        MAP_SCANCODE_TO_STR(PRINTSCREEN, print);
        MAP_SCANCODE_TO_STR(SCROLLLOCK, scroll);
        MAP_SCANCODE_TO_STR(PAUSE, pause);
        MAP_SCANCODE_TO_STR(INSERT, insert);
        MAP_SCANCODE_TO_STR(HOME, home);
        MAP_SCANCODE_TO_STR(PAGEUP, pageup);
        MAP_SCANCODE_TO_STR(DELETE, delete);
        MAP_SCANCODE_TO_STR(END, end);
        MAP_SCANCODE_TO_STR(PAGEDOWN, pagedown);
        MAP_SCANCODE_TO_STR(RIGHT, right);
        MAP_SCANCODE_TO_STR(LEFT, left);
        MAP_SCANCODE_TO_STR(DOWN, down);
        MAP_SCANCODE_TO_STR(UP, up);
        MAP_SCANCODE_TO_STR(KP_0, insert);
        MAP_SCANCODE_TO_STR(KP_1, end);
        MAP_SCANCODE_TO_STR(KP_2, down);
        MAP_SCANCODE_TO_STR(KP_3, next);
        MAP_SCANCODE_TO_STR(KP_4, left);
        MAP_SCANCODE_TO_STR(KP_6, right);
        MAP_SCANCODE_TO_STR(KP_7, home);
        MAP_SCANCODE_TO_STR(KP_8, up);
        MAP_SCANCODE_TO_STR(KP_9, prior);
      default:
        return "";
    }
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

} // namespace Graphics
} // namespace Sirikata

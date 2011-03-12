/*  Sirikata
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

namespace Sirikata {
namespace SimpleCamera {

namespace {

bool anyBool(const boost::any& a) {
    assert(a.type() == typeid(bool));
    return boost::any_cast<bool>(a);
}

Invokable::Dict anyInvokableDict(const boost::any& a) {
    assert(a.type() == typeid(Invokable::Dict));
    return boost::any_cast<Invokable::Dict>(a);
}

String anyString(const boost::any& a) {
    assert(a.type() == typeid(String));
    return boost::any_cast<String>(a);
}

int32 anyInt32(const boost::any& a) {
    assert(a.type() == typeid(int32));
    return boost::any_cast<int32>(a);
}

uint32 anyUInt32(const boost::any& a) {
    assert(a.type() == typeid(uint32));
    return boost::any_cast<uint32>(a);
}

float anyFloat(const boost::any& a) {
    assert(a.type() == typeid(float));
    return boost::any_cast<float>(a);
}

}

InputBindingEvent::InputBindingEvent()
{
}

InputBindingEvent::InputBindingEvent(const InputBindingEvent& other)
 : mEvent(other.mEvent)
{
}

InputBindingEvent::InputBindingEvent(const boost::any& evt)
 : mEvent(anyInvokableDict(evt))
{
}

InputBindingEvent::~InputBindingEvent() {
}


bool InputBindingEvent::valid() const {
    return (!mEvent.empty() &&
        mEvent.find("msg") != mEvent.end());
}

bool InputBindingEvent::isKey() const {
    return (
        anyString(mEvent.find("msg")->second).find("button") != String::npos ||
        anyString(mEvent.find("msg")->second).find("key") != String::npos
    );
}

String InputBindingEvent::keyButton() const {
    assert(isKey());
    return anyString(mEvent.find("button")->second);
}

bool InputBindingEvent::keyPressed() const {
    assert(isKey());
    return (anyString(mEvent.find("msg")->second) == "button-pressed");
}

bool InputBindingEvent::keyReleased() const {
    assert(isKey());
    if (anyString(mEvent.find("msg")->second) == "button-up") return true;
    return false;
}

InputBindingEvent::Modifier InputBindingEvent::keyModifiers() const {
    assert(isKey());
    Modifier out = NONE;

    Invokable::Dict::const_iterator mods_it = mEvent.find("modifier");
    if (mods_it == mEvent.end()) return out;

    Invokable::Dict mods = anyInvokableDict(mods_it->second);
    if ( anyBool(mods.find("shift")->second) )
        out = (Modifier)(out | SHIFT);
    if ( anyBool(mods.find("ctrl")->second) )
        out = (Modifier)(out | CTRL);
    if ( anyBool(mods.find("alt")->second) )
        out = (Modifier)(out | ALT);
    if ( anyBool(mods.find("super")->second) )
        out = (Modifier)(out | SUPER);

    return out;
}


bool InputBindingEvent::isMouseClick() const {
    return anyString(mEvent.find("msg")->second).find("mouse-click") != String::npos;
}

int32 InputBindingEvent::mouseClickButton() const {
    assert(isMouseClick());
    return anyInt32(mEvent.find("button")->second);
}


bool InputBindingEvent::isMouseDrag() const {
    return anyString(mEvent.find("msg")->second).find("mouse-drag") != String::npos;
}

int32 InputBindingEvent::mouseDragButton() const {
    assert(isMouseDrag());
    return anyInt32(mEvent.find("button")->second);
}


float InputBindingEvent::mouseX() const {
    assert(isMouseClick() || isMouseDrag());
    return anyFloat(mEvent.find("x")->second);
}

float InputBindingEvent::mouseY() const {
    assert(isMouseClick() || isMouseDrag());
    return anyFloat(mEvent.find("y")->second);
}

bool InputBindingEvent::isAxis() const {
    return anyString(mEvent.find("msg")->second).find("axis") != String::npos;
}

uint32 InputBindingEvent::axisIndex() const {
    assert(isAxis());
    return anyUInt32(mEvent.find("axis")->second);
}

float InputBindingEvent::axisValue() const {
    assert(isAxis());
    return anyFloat(mEvent.find("value")->second);
}

bool InputBindingEvent::isWeb() const {
    return anyString(mEvent.find("msg")->second).find("webview") != String::npos;
}

String InputBindingEvent::webViewName() const {
    assert(isWeb());
    return anyString(mEvent.find("webview")->second);
}

String InputBindingEvent::webName() const {
    assert(isWeb());
    return anyString(mEvent.find("name")->second);
}



InputBindingEvent& InputBindingEvent::operator=(const InputBindingEvent& rhs) {
    mEvent = rhs.mEvent;
    return *this;
}


String InputBindingEvent::keyModifiersAsString(Modifier m) {
    String ret;
    if (m & SHIFT)
        ret += "shift-";
    if (m & CTRL)
        ret += "ctrl-";
    if (m & ALT)
        ret += "alt-";
    if (m & SUPER)
        ret += "super-";
    return ret;
}

InputBindingEvent::Modifier InputBindingEvent::keyModifiersFromString(const String& s) {
    Modifier out = NONE;
    if ( s.find("shift") != String::npos )
        out = (Modifier)(out | SHIFT);
    if ( s.find("ctrl") != String::npos )
        out = (Modifier)(out | CTRL);
    if ( s.find("alt") != String::npos )
        out = (Modifier)(out | ALT);
    if ( s.find("super") != String::npos )
        out = (Modifier)(out | SUPER);
    return out;
}

String InputBindingEvent::toString() const {
    if (isKey())
    {
        String result = String("key-");
        String mods = keyModifiersAsString(keyModifiers());
        if (!mods.empty()) result += (mods + "-");
        result += keyButton();
        return result;
    }
    else if (isMouseClick()) {
        return "click-" + boost::lexical_cast<String>(mouseClickButton());
    }
    else if (isMouseDrag()) {
        return "drag-" + boost::lexical_cast<String>(mouseDragButton());
    }
    else if (isAxis()) {
        return "axis-" + boost::lexical_cast<String>(axisIndex());
    }
    else if (isWeb()) {
        return (String("web-") + webViewName() + String("-") + webName());
    }
    return "";
}

InputBindingEvent InputBindingEvent::fromString(const String& asString) {
    // First, split into sections by -'s
    std::vector<String> parts;
    int32 last_idx = -1, next_idx = 0;
    while(next_idx != (int)String::npos) {
        next_idx = asString.find('-', last_idx+1);
        String part = asString.substr(last_idx+1, (next_idx == (int)String::npos ? String::npos : next_idx-last_idx-1));
        parts.push_back(part);
        last_idx = next_idx;
    }

    if (parts.empty()) return InputBindingEvent();
    String major = *parts.begin();
    parts.erase(parts.begin());

    if (major == "key") {
        Modifier mod = keyModifiersFromString(asString);
        String but = parts.back();
        return Key(but, mod);
    }
    else if (major == "click") {
        int32 but = boost::lexical_cast<int32>(parts[1]);
        return MouseClick(but);
    }
    else if (major == "drag") {
        int32 but = boost::lexical_cast<int32>(parts[1]);
        return MouseDrag(but);
    }
    else if (major == "axis") {
        uint32 idx = boost::lexical_cast<uint32>(parts[1]);
        return Axis(idx);
    }
    else if (major == "web") {
        assert(parts.size() == 2);
        return Web(parts[0], parts[1]);
    }

    return InputBindingEvent();
}

int32 InputBindingEvent::typeTag() const {
    if (isKey()) return 1;
    if (isMouseClick()) return 2;
    if (isMouseDrag()) return 3;
    if (isAxis()) return 4;
    if (isWeb()) return 5;
    return -1;
}

bool InputBindingEvent::matches(const InputBindingEvent& rhs) const {
    if (typeTag() != rhs.typeTag()) return false;

    if (isKey()) {
        return ((keyButton() == rhs.keyButton()) && (keyModifiers() == rhs.keyModifiers()));
    }
    else if (isMouseClick()) {
        return (mouseClickButton() == rhs.mouseClickButton());
    }
    else if (isMouseDrag()) {
        return (mouseDragButton() == rhs.mouseDragButton());
    }
    else if (isAxis()) {
        return (axisIndex() == rhs.axisIndex());
    }
    else if (isWeb()) {
        return (webViewName() == rhs.webViewName() && webName() == rhs.webName());
    }
    assert(false);
    return true;
}

bool InputBindingEvent::operator<(const InputBindingEvent& rhs) const {
    if (typeTag() != rhs.typeTag())
        return (typeTag() < rhs.typeTag());

    if (isKey()) {
        return (keyButton() < rhs.keyButton() ||
            (keyButton() < rhs.keyButton() && keyModifiers() < rhs.keyModifiers())
        );
    }
    else if (isMouseClick()) {
        return (mouseClickButton() < rhs.mouseClickButton());
    }
    else if (isMouseDrag()) {
        return (mouseDragButton() < rhs.mouseDragButton());
    }
    else if (isAxis()) {
        return (axisIndex() < rhs.axisIndex());
    }
    else if (isWeb()) {
        return (webViewName() < rhs.webViewName() ||
            (webViewName() == rhs.webViewName() && webName() < rhs.webName())
        );
    }
    assert(false);
    return true;
}

std::ostream& operator<<(std::ostream& os, const Sirikata::SimpleCamera::InputBindingEvent& ibe) {
    os << ibe.toString();
    return os;
}

std::istream& operator>>(std::istream& is, Sirikata::SimpleCamera::InputBindingEvent& ibe) {
    // We're assuming that these are separated as strings are. We could use a
    // more complicated approach that takes in more -xyz- sections and tries to
    // parse them to allow these to be used inline with other items, but we'd
    // need some sort of delimiter or two of these next to each other wouldn't
    // parse.
    Sirikata::String internal;
    is >> internal;
    ibe = Sirikata::SimpleCamera::InputBindingEvent::fromString(internal);
    return is;
}

InputBindingEvent InputBindingEvent::Key(String button, Modifier mod) {
    Invokable::Dict data;
    data["msg"] = String("key");
    data["button"] = button;
    // FIXME modifiers
    return InputBindingEvent(data);
}

InputBindingEvent InputBindingEvent::MouseClick(int32 button) {
    Invokable::Dict data;
    data["msg"] = String("mouse-click");
    data["button"] = button;
    return InputBindingEvent(data);
}

InputBindingEvent InputBindingEvent::MouseDrag(int32 button) {
    Invokable::Dict data;
    data["msg"] = String("mouse-drag");
    data["button"] = button;
    return InputBindingEvent(data);
}

InputBindingEvent InputBindingEvent::Axis(uint32 index) {
    Invokable::Dict data;
    data["msg"] = String("axis");
    data["index"] = index;
    return InputBindingEvent(data);
}

InputBindingEvent InputBindingEvent::Web(const String& wvname, const String& name) {
    Invokable::Dict data;
    data["msg"] = String("webview");
    data["webview"] = wvname;
    data["name"] = name;
    return InputBindingEvent(data);
}

} // namespace SimpleCamera
} // namespace Sirikata

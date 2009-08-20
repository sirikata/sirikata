/*  Sirikata liboh -- Ogre Graphics Plugin
 *  InputEventDescriptor.cpp
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

#include "InputEventDescriptor.hpp"

namespace Sirikata {
namespace Input {

EventDescriptor EventDescriptor::Key(KeyButton button, KeyEvent type, Modifier mod) {
    EventDescriptor result;
    result.mTag = KeyEventTag;
    result.mDescriptor.key.button = button;
    result.mDescriptor.key.type = type;
    result.mDescriptor.key.mod = mod;
    return result;
}

EventDescriptor EventDescriptor::MouseHover() {
    EventDescriptor result;
    result.mTag = MouseHoverEventTag;
    return result;
}

EventDescriptor EventDescriptor::MouseClick(MouseButton button) {
    EventDescriptor result;
    result.mTag = MouseClickEventTag;
    result.mDescriptor.mouseClick.button = button;
    return result;
}

EventDescriptor EventDescriptor::MousePressed(MouseButton button) {
    EventDescriptor result;
    result.mTag = MousePressedEventTag;
    result.mDescriptor.mousePressed.button = button;
    return result;
}

EventDescriptor EventDescriptor::MouseDrag(MouseButton button, MouseDragType type) {
    EventDescriptor result;
    result.mTag = MouseDragEventTag;
    result.mDescriptor.mouseDrag.button = button;
    result.mDescriptor.mouseDrag.type = type;
    return result;
}

EventDescriptor EventDescriptor::Axis(AxisIndex index) {
    EventDescriptor result;
    result.mTag = AxisEventTag;
    result.mDescriptor.axis.index = index;
    return result;
}

EventDescriptor EventDescriptor::Text() {
    EventDescriptor result;
    result.mTag = TextEventTag;
    return result;
}

EventDescriptor EventDescriptor::Window(WindowEventType type) {
    EventDescriptor result;
    result.mTag = WindowEventTag;
    result.mDescriptor.window.type = type;
    return result;
}

EventDescriptor EventDescriptor::DragAndDrop() {
    EventDescriptor result;
    result.mTag = DragAndDropEventTag;
    return result;
}

EventDescriptor EventDescriptor::Web(const String& wvname, const String& name, uint32 argcount) {
    EventDescriptor result;
    result.mTag = WebEventTag;
    result.mDescriptor.web.wvname = new String(wvname);
    result.mDescriptor.web.name = new String(name);
    result.mDescriptor.web.argcount = argcount;
    return result;
}

EventDescriptor::EventDescriptor()
 : mTag(Bogus)
{
}

EventDescriptor::EventDescriptor(const EventDescriptor& other) {
    *this = other;
}

EventDescriptor::~EventDescriptor() {
    if (isWeb()) {
        delete mDescriptor.web.wvname;
        mDescriptor.web.wvname = NULL;
        delete mDescriptor.web.name;
        mDescriptor.web.name = NULL;
    }
}

bool EventDescriptor::isKey() const {
    return mTag == KeyEventTag;
}

KeyButton EventDescriptor::keyButton() const {
    assert(isKey());
    return mDescriptor.key.button;
}

KeyEvent EventDescriptor::keyEvents() const {
    assert(isKey());
    return mDescriptor.key.type;
}

Modifier EventDescriptor::keyModifiers() const {
    assert(isKey());
    return mDescriptor.key.mod;
}


bool EventDescriptor::isMouseClick() const {
    return mTag == MouseClickEventTag;
}

MouseButton EventDescriptor::mouseClickButton() const {
    assert(isMouseClick());
    return mDescriptor.mouseClick.button;
}

bool EventDescriptor::isMousePressed() const {
    return mTag == MousePressedEventTag;
}

MouseButton EventDescriptor::mousePressedButton() const {
    assert(isMousePressed());
    return mDescriptor.mousePressed.button;
}


bool EventDescriptor::isMouseDrag() const {
    return mTag == MouseDragEventTag;
}

MouseButton EventDescriptor::mouseDragButton() const {
    assert(isMouseDrag());
    return mDescriptor.mouseDrag.button;
}

MouseDragType EventDescriptor::mouseDragType() const {
    assert(isMouseDrag());
    return mDescriptor.mouseDrag.type;
}


bool EventDescriptor::isAxis() const {
    return mTag == AxisEventTag;
}

AxisIndex EventDescriptor::axisIndex() const {
    assert(isAxis());
    return mDescriptor.axis.index;
}

bool EventDescriptor::isWeb() const {
    return mTag == WebEventTag;
}

const String& EventDescriptor::webViewName() const {
    assert(isWeb());
    return *mDescriptor.web.wvname;
}

const String& EventDescriptor::webName() const {
    assert(isWeb());
    return *mDescriptor.web.name;
}

uint32 EventDescriptor::webArgCount() const {
    assert(isWeb());
    return mDescriptor.web.argcount;
}


bool EventDescriptor::operator<(const EventDescriptor& rhs) const {
    // Evaluate easy comparisons first
    if (mTag < rhs.mTag) return true;
    if (rhs.mTag < mTag) return false;

    // Otherwise, types are the same so we need to do detailed checks
    if (mTag == KeyEventTag)
        return
            mDescriptor.key.button < rhs.mDescriptor.key.button ||
            (mDescriptor.key.button == rhs.mDescriptor.key.button &&
                (mDescriptor.key.type < rhs.mDescriptor.key.type ||
                (mDescriptor.key.type == rhs.mDescriptor.key.type &&
                    mDescriptor.key.mod < rhs.mDescriptor.key.mod
                )
                )
            );

    if (mTag == MouseHoverEventTag)
        return false;

    if (mTag == MouseClickEventTag)
        return mDescriptor.mouseClick.button < rhs.mDescriptor.mouseClick.button;

    if (mTag == MouseDragEventTag)
        return mDescriptor.mouseDrag.button < rhs.mDescriptor.mouseDrag.button ||
            (mDescriptor.mouseDrag.button == rhs.mDescriptor.mouseDrag.button &&
                (mDescriptor.mouseDrag.type < rhs.mDescriptor.mouseDrag.type)
            );

    if (mTag == AxisEventTag)
        return mDescriptor.axis.index < rhs.mDescriptor.axis.index;

    if (mTag == TextEventTag)
        return false;

    if (mTag == WindowEventTag)
        return mDescriptor.window.type < rhs.mDescriptor.window.type;

    if (mTag == DragAndDropEventTag)
        return false;

    if (mTag == WebEventTag) {
        return mDescriptor.web.wvname->compare(*rhs.mDescriptor.web.wvname) < 0 ||
            (mDescriptor.web.wvname->compare(*rhs.mDescriptor.web.wvname) == 0 &&
                (mDescriptor.web.name->compare(*rhs.mDescriptor.web.name) < 0 ||
                    (mDescriptor.web.name->compare(*rhs.mDescriptor.web.name) == 0 &&
                        mDescriptor.web.argcount < rhs.mDescriptor.web.argcount
                    )
                )
            );
    }

    assert(false); // we should have checked all types of tags by now
}

EventDescriptor& EventDescriptor::operator=(const EventDescriptor& rhs) {
    mTag = rhs.mTag;
    mDescriptor = rhs.mDescriptor;

    // We need to make a copy of the strings
    if (isWeb()) {
        mDescriptor.web.wvname = new String(*rhs.mDescriptor.web.wvname);
        mDescriptor.web.name = new String(*rhs.mDescriptor.web.name);
    }

    return *this;
}

} // namespace Input
} // namespace Sirikata

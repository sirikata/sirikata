/*  Sirikata libproxyobject -- Ogre Graphics Plugin
 *  InputEventDescriptor.hpp
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

#ifndef _SIRIKATA_INPUT_EVENT_DESCRIPTOR_HPP_
#define _SIRIKATA_INPUT_EVENT_DESCRIPTOR_HPP_

#include <sirikata/core/util/Platform.hpp>
#include "InputDevice.hpp"

namespace Sirikata {
namespace Input {

enum EventTypeTag {
    Bogus = 0,
    KeyEventTag = 1,
    MouseHoverEventTag = 2,
    MousePressedEventTag = 3,
    MouseClickEventTag = 4,
    MouseDragEventTag = 5,
    AxisEventTag = 6,
    TextEventTag = 7,
    WindowEventTag = 8,
    DragAndDropEventTag = 9,
    WebEventTag = 10
};

class EventDescriptor {
public:
    static EventDescriptor Key(KeyButton button, KeyEvent type = KEY_PRESSED, Modifier mod = MOD_NONE);
    static EventDescriptor MouseHover();
    static EventDescriptor MouseClick(MouseButton button);
    static EventDescriptor MousePressed(MouseButton button);
    static EventDescriptor MouseDrag(MouseButton button, MouseDragType type);
    static EventDescriptor Axis(AxisIndex axis);
    static EventDescriptor Text();
    static EventDescriptor Window(WindowEventType type);
    static EventDescriptor DragAndDrop();
    static EventDescriptor Web(const String& wvname, const String& name);

    EventDescriptor();
    EventDescriptor(const EventDescriptor& other);
    ~EventDescriptor();

    bool isKey() const;
    KeyButton keyButton() const;
    KeyEvent keyEvents() const;
    Modifier keyModifiers() const;

    bool isMouseClick() const;
    MouseButton mouseClickButton() const;

    bool isMousePressed() const;
    MouseButton mousePressedButton() const;

    bool isMouseDrag() const;
    MouseButton mouseDragButton() const;
    MouseDragType mouseDragType() const;

    bool isAxis() const;
    AxisIndex axisIndex() const;

    bool isWeb() const;
    const String& webViewName() const;
    const String& webName() const;

    bool operator<(const EventDescriptor& rhs) const;
    EventDescriptor& operator=(const EventDescriptor& rhs);
private:
    EventTypeTag mTag;

    union {
        struct {
            KeyButton button;
            KeyEvent type;
            Modifier mod;
        } key;
        struct {
            MouseButton button;
        } mouseClick;
        struct {
            MouseButton button;
        } mousePressed;
        struct {
            MouseButton button;
            MouseDragType type;
        } mouseDrag;
        struct {
            AxisIndex index;
        } axis;
        struct {
            WindowEventType type;
        } window;
        struct {
            String* wvname;
            String* name;
        } web;
    } mDescriptor;
};

} // namespace Input
} // namespace Sirikata

#endif //_SIRIKATA_INPUT_EVENT_DESCRIPTOR_HPP_

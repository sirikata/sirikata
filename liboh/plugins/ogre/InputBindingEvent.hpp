/*  Sirikata liboh -- Ogre Graphics Plugin
 *  InputBindingEvent.hpp
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

#ifndef _SIRIKATA_INPUT_BINDING_EVENT_HPP_
#define _SIRIKATA_INPUT_BINDING_EVENT_HPP_

#include <util/Platform.hpp>
#include "input/InputEvents.hpp"
#include "input/InputEventDescriptor.hpp"

namespace Sirikata {
namespace Graphics {

class InputBindingEvent {
public:
    enum TypeTag {
        KeyEventTag = 1,
        MouseClickEventTag = 2,
        MouseDragEventTag = 3,
        AxisEventTag = 4
    };

    typedef Input::EventDescriptor::KeyEventButton KeyEventButton;
    typedef Input::EventDescriptor::KeyEventModifier KeyEventModifier;
    static const KeyEventModifier KeyModNone  = Input::EventDescriptor::KeyModNone;
    static const KeyEventModifier KeyModAlt   = Input::EventDescriptor::KeyModAlt;
    static const KeyEventModifier KeyModCtrl  = Input::EventDescriptor::KeyModCtrl;
    static const KeyEventModifier KeyModShift = Input::EventDescriptor::KeyModShift;

    typedef Input::EventDescriptor::MouseClickEventButton MouseClickEventButton;

    typedef Input::EventDescriptor::MouseDragEventButton MouseDragEventButton;

    typedef Input::EventDescriptor::AxisIndex AxisIndex;

    static InputBindingEvent Key(KeyEventButton button, KeyEventModifier mod = KeyModNone);
    static InputBindingEvent MouseClick(MouseClickEventButton button);
    static InputBindingEvent MouseDrag(MouseDragEventButton button);
    static InputBindingEvent Axis(AxisIndex axis);

    bool isKey() const;
    KeyEventButton keyButton() const;
    KeyEventModifier keyModifiers() const;

    bool isMouseClick() const;
    MouseClickEventButton mouseClickButton() const;

    bool isMouseDrag() const;
    MouseDragEventButton mouseDragButton() const;

    bool isAxis() const;
    AxisIndex axisIndex() const;

private:
    TypeTag mTag;

    union {
        struct {
            KeyEventButton button;
            KeyEventModifier mod;
        } key;
        struct {
            MouseClickEventButton button;
        } mouseClick;
        struct {
            MouseDragEventButton button;
        } mouseDrag;
        struct {
            AxisIndex index;
        } axis;
    } mDescriptor;
};

} // namespace Graphics
} // namespace Sirikata

#endif //_SIRIKATA_INPUT_BINDING_EVENT_HPP_

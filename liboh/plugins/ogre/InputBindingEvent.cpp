/*  Sirikata liboh -- Ogre Graphics Plugin
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

namespace Sirikata {
namespace Graphics {

InputBindingEvent InputBindingEvent::Key(KeyEventButton button, KeyEventModifier mod) {
    InputBindingEvent result;
    result.mTag = KeyEventTag;
    result.mDescriptor.key.button = button;
    result.mDescriptor.key.mod = mod;
    return result;
}

InputBindingEvent InputBindingEvent::MouseClick(MouseClickEventButton button) {
    InputBindingEvent result;
    result.mTag = MouseClickEventTag;
    result.mDescriptor.mouseClick.button = button;
    return result;
}

InputBindingEvent InputBindingEvent::MouseDrag(MouseDragEventButton button) {
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

bool InputBindingEvent::isKey() const {
    return mTag == KeyEventTag;
}

InputBindingEvent::KeyEventButton InputBindingEvent::keyButton() const {
    assert(isKey());
    return mDescriptor.key.button;
}

InputBindingEvent::KeyEventModifier InputBindingEvent::keyModifiers() const {
    assert(isKey());
    return mDescriptor.key.mod;
}

bool InputBindingEvent::isMouseClick() const {
    return mTag == MouseClickEventTag;
}

InputBindingEvent::MouseClickEventButton InputBindingEvent::mouseClickButton() const {
    assert(isMouseClick());
    return mDescriptor.mouseClick.button;
}

bool InputBindingEvent::isMouseDrag() const {
    return mTag == MouseDragEventTag;
}

InputBindingEvent::MouseDragEventButton InputBindingEvent::mouseDragButton() const {
    assert(isMouseDrag());
    return mDescriptor.mouseDrag.button;
}

bool InputBindingEvent::isAxis() const {
    return mTag == AxisEventTag;
}

InputBindingEvent::AxisIndex InputBindingEvent::axisIndex() const {
    assert(isAxis());
    return mDescriptor.axis.index;
}

} // namespace Graphics
} // namespace Sirikata

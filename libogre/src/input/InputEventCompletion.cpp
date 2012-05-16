// Copyright (c) 2012 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include <sirikata/ogre/input/InputEventCompletion.hpp>

namespace Sirikata {
namespace Input {

InputEventCompletion::InputEventCompletion()
 : mTarget(NULL)
{
}

void InputEventCompletion::onKeyPressedEvent(ButtonPressedPtr ev) {
    mKeyEvents[ev->mButton] = ev;
}

void InputEventCompletion::onKeyRepeatedEvent(ButtonRepeatedPtr ev) {
    mKeyEvents[ev->mButton] = ev;
}

void InputEventCompletion::onKeyReleasedEvent(ButtonReleasedPtr ev) {
    if (mKeyEvents.find(ev->mButton) == mKeyEvents.end())
        return;

    mKeyEvents.erase(ev->mButton);
}

void InputEventCompletion::onMousePressedEvent(MousePressedEventPtr ev) {
    mMouseEvents[ev->mButton] = ev;
}

void InputEventCompletion::onMouseDragEvent(MouseDragEventPtr ev) {
    mMouseEvents[ev->mButton] = ev;
}

void InputEventCompletion::onMouseReleasedEvent(MouseReleasedEventPtr ev) {
    if (mMouseEvents.find(ev->mButton) == mMouseEvents.end())
        return;

    mMouseEvents.erase(ev->mButton);
}

void InputEventCompletion::updateTarget(InputListener* target) {
    // If the target is the same, we can just continue normally. This check
    // allows the user to just always updateTarget()
    if (target == mTarget) return;
    // Generate key up events
    for(std::map<KeyButton, ButtonEventPtr>::iterator it = mKeyEvents.begin(); it != mKeyEvents.end(); it++) {
        mTarget->onKeyReleasedEvent(
            ButtonReleasedPtr(
                new ButtonReleased(it->second->getDevice(), it->second->mButton, it->second->mModifier)
            )
        );
    }

    // Generate mouse up events
    for(std::map<MouseButton, MouseDownEventPtr>::iterator it = mMouseEvents.begin(); it != mMouseEvents.end(); it++) {
        mTarget->onMouseReleasedEvent(
            MouseReleasedEventPtr(
                new MouseReleasedEvent(it->second->getDevice(), it->second->mX, it->second->mY, it->second->mCursorType, it->second->mButton)
            )
        );
    }

    // And finally, clear out the old events and replace the listener
    mKeyEvents.clear();
    mMouseEvents.clear();
    mTarget = target;
}

} // namespace Input
} // namespace Sirikata

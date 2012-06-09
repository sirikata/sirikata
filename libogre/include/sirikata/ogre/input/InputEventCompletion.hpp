// Copyright (c) 2012 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_OGRE_INPUT_EVENT_COMPLETION_HPP_
#define _SIRIKATA_OGRE_INPUT_EVENT_COMPLETION_HPP_

#include <sirikata/ogre/input/InputListener.hpp>
#include <sirikata/ogre/input/InputEvents.hpp>

namespace Sirikata {
namespace Input {

/** Tracks input events to ensure coherent streams of events when dispatching
 *  a single stream of input events to multiple consumers. For example, if you
 *  press a key down, then click a mouse such that the input focus changes, this
 *  allows you to generate a fake key up event when the focus changes so the
 *  element that originally had focus isn't left without a key up event.
 */
class SIRIKATA_OGRE_EXPORT InputEventCompletion {
public:
    InputEventCompletion();

    void onKeyPressedEvent(ButtonPressedPtr ev);
    void onKeyRepeatedEvent(ButtonRepeatedPtr ev);
    void onKeyReleasedEvent(ButtonReleasedPtr ev);
    void onMousePressedEvent(MousePressedEventPtr ev);
    void onMouseDragEvent(MouseDragEventPtr ev);
    void onMouseReleasedEvent(MouseReleasedEventPtr ev);

    /** Update the target listener that events are being handled by. Any
     *  outstanding logical events will have completion events generate and
     *  delivered to the old listener, e.g. a key up and a mouse up if we still
     *  had a key and mouse button pressed.
     */
    void updateTarget(InputListener* target);

private:
    // The target currently receiving events
    InputListener* mTarget;
    // Tracking of outstanding events. We only need to store "down" events, but
    // we have to track multiple since we could have multiple keys or mouse
    // buttons depressed.
    std::map<KeyButton, ButtonEventPtr> mKeyEvents;
    std::map<MouseButton, MouseDownEventPtr> mMouseEvents;
}; //class InputEventCompletion

} // namespace Input
} // namespace Sirikata

#endif //_SIRIKATA_OGRE_INPUT_EVENT_COMPLETION_HPP_

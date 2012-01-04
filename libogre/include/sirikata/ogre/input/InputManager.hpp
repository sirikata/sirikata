/*  Sirikata Input Plugin -- plugins/input
 *  InputManager.hpp
 *
 *  Copyright (c) 2009, Patrick Reiter Horn
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

#ifndef SIRIKATA_INPUT_InputManager_HPP__
#define SIRIKATA_INPUT_InputManager_HPP__

#include <sirikata/ogre/input/InputDevice.hpp>
#include <sirikata/ogre/input/InputListener.hpp>
#include <sirikata/core/util/ListenerProvider.hpp>

namespace Sirikata {
namespace Input {

typedef std::tr1::weak_ptr<InputDevice> InputDeviceWPtr;

struct SIRIKATA_OGRE_EXPORT InputDeviceEvent {
    enum Type {ADDED, REMOVED} mType;
    InputDevicePtr mDevice;
    InputDeviceEvent(Type type, const InputDevicePtr &dev)
        : mType(type), mDevice(dev) {
    }
};

class SIRIKATA_OGRE_EXPORT InputManager : public Provider<InputListener*> {
protected:
    std::set<InputDevicePtr> mAllDevices;
public:
    InputManager() {}
    virtual ~InputManager() {}

	virtual void getWindowSize(unsigned int &width, unsigned int &height) = 0; // temporary

    virtual bool isModifierDown(Modifier modifier) const = 0;
    virtual bool isCapsLockDown() const = 0;
    virtual bool isNumLockDown() const = 0;
    virtual bool isScrollLockDown() const = 0;

    // Helpers for firing events to listeners
    virtual void fire(InputDeviceEventPtr ev) { notify(&InputListener::onInputDeviceEvent, ev); }
    virtual void fire(ButtonPressedPtr ev) { notify(&InputListener::onKeyPressedEvent, ev); }
    virtual void fire(ButtonRepeatedPtr ev) { notify(&InputListener::onKeyRepeatedEvent, ev); }
    virtual void fire(ButtonReleasedPtr ev) { notify(&InputListener::onKeyReleasedEvent, ev); }
    virtual void fire(ButtonDownPtr ev) { notify(&InputListener::onKeyDownEvent, ev); }
    virtual void fire(AxisEventPtr ev) { notify(&InputListener::onAxisEvent, ev); }
    virtual void fire(TextInputEventPtr ev) { notify(&InputListener::onTextInputEvent, ev); }
    virtual void fire(MouseHoverEventPtr ev) { notify(&InputListener::onMouseHoverEvent, ev); }
    virtual void fire(MousePressedEventPtr ev) { notify(&InputListener::onMousePressedEvent, ev); }
    virtual void fire(MouseReleasedEventPtr ev) { notify(&InputListener::onMouseReleasedEvent, ev); }
    virtual void fire(MouseClickEventPtr ev) { notify(&InputListener::onMouseClickEvent, ev); }
    virtual void fire(MouseDragEventPtr ev) { notify(&InputListener::onMouseDragEvent, ev); }
    virtual void fire(DragAndDropEventPtr ev) { notify(&InputListener::onDragAndDropEvent, ev); }
    virtual void fire(WebViewEventPtr ev) { notify(&InputListener::onWebViewEvent, ev); }
    virtual void fire(WindowEventPtr ev) { notify(&InputListener::onWindowEvent, ev); }
};

}
}

#endif

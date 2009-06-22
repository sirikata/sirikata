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

#include <task/EventManager.hpp>
#include "InputDevice.hpp"

namespace Sirikata {
namespace Input {

typedef std::tr1::weak_ptr<InputDevice> InputDeviceWPtr;

struct InputDeviceEvent : public Task::Event {
    typedef Task::IdPair IdPair;
    static const IdPair::Primary &getEventId() {
        static IdPair::Primary retval("InputDeviceEvent");
        return retval;
    }

    enum Type {ADDED, REMOVED} mType;
    InputDevicePtr mDevice;
    InputDeviceEvent(Type type, const InputDevicePtr &dev)
        : Task::Event(IdPair(getEventId(), IdPair::Secondary::null())),
         mType(type), mDevice(dev) {
    }
};

class InputManager : public Task::GenEventManager {
protected:
    std::set<InputDevicePtr> mAllDevices;
public:
    InputManager(Task::WorkQueue*queue) : Task::GenEventManager(queue) {}
    virtual ~InputManager() {}

	virtual void getWindowSize(unsigned int &width, unsigned int &height) = 0; // temporary

    virtual bool isModifierDown(int modifier) const = 0;
    virtual bool isCapsLockDown() const = 0;
    virtual bool isNumLockDown() const = 0;
    virtual bool isScrollLockDown() const = 0;

    /** Calls func *synchronously* for each device already known, then
        registers DeviceAdded and DeviceRemoved events.
     Note: SDL does not yet support adding or removing devices. */
    Task::SubscriptionId registerDeviceListener(const EventListener &func) {
        for (std::set<InputDevicePtr>::iterator iter = mAllDevices.begin();
             iter != mAllDevices.end();
             ++iter) {
            func(EventPtr(new InputDeviceEvent(InputDeviceEvent::ADDED, (*iter))));
        }
        return subscribeId(InputDeviceEvent::getEventId(), func);
    }
 
    void addDevice(const InputDevicePtr &dev) {
        mAllDevices.insert(dev);
        fire(EventPtr(new InputDeviceEvent(InputDeviceEvent::ADDED, dev)));
    }
 
    void removeDevice(const InputDevicePtr &dev) {
        std::set<InputDevicePtr>::iterator iter = mAllDevices.find(dev);
        if (iter != mAllDevices.end()) {
            mAllDevices.erase(iter);
        }
        fire(EventPtr(new InputDeviceEvent(InputDeviceEvent::REMOVED, dev)));
    }
};

}
}

#endif

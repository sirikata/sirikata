/*  Sirikata Input Plugin -- plugins/input
 *  InputDevice.cpp
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

#include <util/Standard.hh>
#include <task/Event.hpp>
#include <task/EventManager.hpp>
#include "InputEvents.hpp"
#include "InputDevice.hpp"

namespace Sirikata {
namespace Input {

using Task::EventPtr;
using Task::GenEventManager;

bool InputDevice::buttonChanged(unsigned int button, bool newState, Modifier &modifiers) {
    bool changed;
    if (newState == true) {
        ButtonSet::iterator iter = buttonState.find(button);
        if (iter != buttonState.end()) {
            if ((*iter).second != modifiers) {
                modifiers = (*iter).second;
                changed = true;
            } else {
                changed = false;
            }
        } else {
            changed = true;
        }
        if (changed) {
            // may have different set of modifiers.
            buttonState.insert(ButtonSet::value_type(button,modifiers));
        }
    } else {
        ButtonSet::iterator iter = buttonState.find(button);
        if (iter != buttonState.end()) {
            modifiers = (*iter).second;
            buttonState.erase(iter);
            changed = true;
        } else {
            changed = false;
        }
    }
    return changed;
}
bool InputDevice::fireButton(const InputDevicePtr &thisptr, 
                             GenEventManager *em, 
                             unsigned int button, bool newState, Modifier modifiers) {
    Modifier oldmodifiers = modifiers;
    bool changed = buttonChanged(button, newState, oldmodifiers);
    if (changed) {
        if (newState) {
            if (oldmodifiers != modifiers) {
                em->fire(EventPtr(new ButtonReleased(thisptr, button, oldmodifiers)));
            }
            em->fire(EventPtr(new ButtonPressed(thisptr, button, modifiers)));
        } else {
            em->fire(EventPtr(new ButtonReleased(thisptr, button, oldmodifiers)));
        }
    }
    return changed;
}

bool InputDevice::axisChanged(unsigned int axis, AxisValue newState) {
    newState.clip();
    while (axisState.size() <= axis) {
        axisState.push_back(AxisValue::null());
    }
    bool changed = (axisState[axis] != newState);
    axisState[axis] = newState;
    return changed;
}

bool InputDevice::fireAxis(const InputDevicePtr &thisptr, 
                           GenEventManager *em,
                           unsigned int axis, AxisValue newState) {
    newState.clip();
    bool changed = axisChanged(axis, newState);
    if (changed) {
        em->fire(EventPtr(new AxisEvent(thisptr, axis, newState)));
    }
    return changed;
}

}
}


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

#include <sirikata/core/util/Standard.hh>
#include "../task/Event.hpp"
#include "../task/EventManager.hpp"
#include "InputEvents.hpp"
#include "InputDevice.hpp"

namespace Sirikata {
namespace Input {

using Task::EventPtr;
using Task::GenEventManager;

bool InputDevice::changeButton(unsigned int button, bool newState, Modifier &modifiers) {
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
                             unsigned int button, bool newStateIsPressed, Modifier modifiers) {
    Modifier oldmodifiers = modifiers;
    bool changed = changeButton(button, newStateIsPressed, oldmodifiers);
    if (changed) {
        if (newStateIsPressed) {
            if (oldmodifiers != modifiers) {
                // If modifiers change, release old
                em->fire(EventPtr(new ButtonReleased(thisptr, button, oldmodifiers)));
            }
            em->fire(EventPtr(new ButtonPressed(thisptr, button, modifiers)));
        } else {
            em->fire(EventPtr(new ButtonReleased(thisptr, button, oldmodifiers)));
        }
    } else {
        if (newStateIsPressed) {
            if (oldmodifiers != modifiers) {
                // If modifiers change, release old and press new
                em->fire(EventPtr(new ButtonReleased(thisptr, button, oldmodifiers)));
                em->fire(EventPtr(new ButtonPressed(thisptr, button, modifiers)));
            }
            else {
                // Otherwise, we're really in repeat mode
                em->fire(EventPtr(new ButtonRepeated(thisptr, button, modifiers)));
            }
        }
        // Otherwise, we're getting repeats when the key is up....
    }
    return changed;
}

bool InputDevice::changeAxis(unsigned int axis, AxisValue newState) {
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
    bool changed = changeAxis(axis, newState);
    changed = changed || (newState != AxisValue::null());
    if (changed) {
        em->fire(EventPtr(new AxisEvent(thisptr, axis, newState)));
    }
    return changed;
}

void PointerDevice::firePointerClick(
        const PointerDevicePtr &thisptr,
        GenEventManager *em,
        float xPixel,
        float yPixel,
        int cursor,
        int button,
        bool state) {
    DragMap::iterator iter = mDragInfo.begin();
    while (iter != mDragInfo.end()) {
        if ((*iter).mButton == button) {
            break;
        }
        ++iter;
    }
    if (iter == mDragInfo.end()) {
        if (!state) return;
        DragInfo di;
        di.mButton = button;
        di.mIsDragging = false;
        di.mDragStartX = xPixel;
        di.mDragStartY = yPixel;
        di.mDragX = xPixel;
        di.mDragY = yPixel;
        di.mOffsetX = 0;
        di.mOffsetY = 0;
        mDragInfo.insert(mDragInfo.begin(), di);
        em->fire(EventPtr(
                new MousePressedEvent(
                    thisptr,
                    xPixel,
                    yPixel,
                    cursor, button)));
    } else {
        if (state) return;
        if ((*iter).mIsDragging) {
            if (mRelativeMode) {
                xPixel = (*iter).mDragX;
                yPixel = (*iter).mDragY;
            }
            em->fire(EventPtr(
                    new MouseDragEvent(
                        thisptr, DRAG_END,
                        (*iter).mDragStartX,
                        (*iter).mDragStartY,
                        xPixel+(*iter).mOffsetX,
                        yPixel+(*iter).mOffsetY,
                        (*iter).mDragX+(*iter).mOffsetX,
                        (*iter).mDragY+(*iter).mOffsetY,
                        cursor, button, 0, 0, 0)));
        } else {
            em->fire(EventPtr(
                    new MouseClickEvent(
                        thisptr,
                        (*iter).mDragStartX,
                        (*iter).mDragStartY,
                        cursor, button)));
            em->fire(EventPtr(
                    new MouseReleasedEvent(
                        thisptr,
                        xPixel,
                        yPixel,
                        cursor, button)));
        }
        mDragInfo.erase(iter);
    }
}

void PointerDevice::firePointerMotion(
        const PointerDevicePtr &thisptr,
        GenEventManager *em,
        float xPixelArg,
        float yPixelArg,
        int cursorType,
        int pressure, int pressmin, int pressmax) {
    if (mDragInfo.empty() && !mRelativeMode) {
        em->fire(EventPtr(new MouseHoverEvent(thisptr, xPixelArg, yPixelArg, cursorType)));
    } else {
        bool first = true;
        for (DragMap::iterator iter = mDragInfo.begin();
             iter != mDragInfo.end(); ++iter) {
            DragInfo &di = (*iter);
            float xPixel = xPixelArg, yPixel = yPixelArg;
            if (mRelativeMode) {
                xPixel = di.mDragX + xPixelArg;
                yPixel = di.mDragY + yPixelArg;
            }
            if (first) {
                MouseDragType dragType = DRAG_DEADBAND;
                if (!di.mIsDragging) {
                    float xdiff = (xPixel - di.mDragStartX);
                    float ydiff = (yPixel - di.mDragStartY);
                    if (xdiff*xdiff + ydiff*ydiff >= mDeadband*mDeadband) {
                        di.mIsDragging = true;
                        dragType = DRAG_START;
                    }
                } else {
                    dragType = DRAG_DRAG;
                }
                em->fire(EventPtr(new MouseDragEvent(
                                      thisptr, dragType,
                                      di.mDragStartX, di.mDragStartY,
                                      xPixel+di.mOffsetX, yPixel+di.mOffsetY,
                                      di.mDragX+di.mOffsetX, di.mDragY+di.mOffsetY,
                                      cursorType, di.mButton,
                                      pressure, pressmin, pressmax)));
                first = false;
                if (mRelativeMode) {
                    di.mOffsetX += xPixelArg;
                    di.mOffsetY += yPixelArg;
                }
            } else if (!mRelativeMode) {
                di.mOffsetX += di.mDragX - xPixel;
                di.mOffsetY += di.mDragY - yPixel;
            }
            di.mDragX = xPixel;
            di.mDragY = yPixel;
        }
    }
}

}
}

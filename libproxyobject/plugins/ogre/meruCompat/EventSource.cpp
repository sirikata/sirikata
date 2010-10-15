/*  Meru
 *  EventSource.cpp
 *
 *  Copyright (c) 2009, Stanford University
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

#include "EventSource.hpp"
#include "SequentialWorkQueue.hpp"
#include <sirikata/core/task/WorkQueue.hpp>

namespace Meru {

::Sirikata::Task::GenEventManager *EventSource::sSingleton;

void EventSource::InitializeEventTypes() {
    EventTypes::WorkQueue = new EventType("WorkQueue");
    EventTypes::Tick = new EventType("Tick");
    EventTypes::AvatarCreated = new EventType("AvatarCreated");
    EventTypes::ObjectDatabaseRestored = new EventType("ObjectDatabaseRestored");

    EventTypes::ObjectCreated = new EventType("ObjectCreated");
    EventTypes::ObjectRestored = new EventType("ObjectRestored");
    EventTypes::ObjectDestroyed = new EventType("ObjectDestroyed");

    EventTypes::NamedProxyCreated = new EventType("NamedProxyCreated");
    EventTypes::NamedProxyDeleted = new EventType("NamedProxyDeleted");
    EventTypes::PositionUpdate = new EventType("PositionUpdate");
    EventTypes::LocalPositionUpdate = new EventType("LocalPositionUpdate");
    EventTypes::ProxySelected = new EventType("ProxySelected");
    EventTypes::ProxyDeselected = new EventType("ProxyDeselected");
    EventTypes::Quit = new EventType("Quit");
}

} // namespace Meru

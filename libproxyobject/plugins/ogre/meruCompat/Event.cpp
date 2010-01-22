/*  Meru
 *  Event.cpp
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

#include "Event.hpp"
#include <boost/lexical_cast.hpp>
namespace Meru {

namespace EventTypes {
const EventType WorkQueue="WorkQueue";
const EventType Tick="Tick";
const EventType AvatarCreated="AvatarCreated";
const EventType ObjectDatabaseRestored="ObjectDatabaseRestored";

const EventType ObjectCreated="ObjectCreated";
const EventType ObjectRestored="ObjectRestored";
const EventType ObjectDestroyed="ObjectDestroyed";

const EventType NamedProxyCreated="NamedProxyCreated";
const EventType NamedProxyDeleted="NamedProxyDeleted";
const EventType PositionUpdate="PositionUpdate";
const EventType LocalPositionUpdate="LocalPositionUpdate";
const EventType ProxySelected="ProxySelected";
const EventType ProxyDeselected="ProxyDeselected";
const EventType Quit="Quit";
}

TickEvent::TickEvent(const Time& t, const Duration& dt)
    : Event(EventID(EventTypes::Tick)),
     mTime(t),
     mDuration(dt)
{
}

String TickEvent::toString() const {
    std::ostringstream os;
    os<<"("<< getId().toString() <<" - ["<< (mTime-Time::null()).toSeconds() << "])";
    return os.str();
}


UserEvent::UserEvent(const EventID& evt_id, const SpaceObjectReference& id)
: Event(evt_id), mObject(id)
{
}

String UserEvent::toString() const {
    return String("(") + getId().toString() + String(" - ") + mObject.toString() + String(")");
}

ObjectEvent::ObjectEvent(const EventID& evt_id, const UUID& id)
: Event(evt_id), mUUID(id)
{
}

String ObjectEvent::toString() const {
    return String("(") + getId().toString() + String(" - ") + mUUID.toString() + String(")");
}

ProximityEvent::ProximityEvent(const EventID& evt_id, const ObjectReference& id1, const ObjectReference& id2)
: Event(evt_id)
{
    mObjects[0] = id1;
    mObjects[1] = id2;
}

String ProximityEvent::toString() const {
    return String("(") + getId().toString() + String(" - [") + mObjects[0].toString() + String(",") + mObjects[1].toString() + String("]") + String(")");
}

PositionUpdate::PositionUpdate(const SpaceObjectReference& id, const Time t, const Vector3d& pos, const Quaternion& orient, const Vector3f& vel)
: Event(EventID(EventTypes::PositionUpdate)), time(t), position(pos), orientation(orient), velocity(vel), mID(id)
{
}

String PositionUpdate::toString() const {
  std::ostringstream os;
  os <<String("(")<< getId().toString()<< String(" - [") << mID.toString() <<
        String(" at ") << (time-Time::null()).toSeconds() << String(", ") <<
        position.toString() << String(", ") << orientation.toString() << String(", ") <<
        velocity.toString() << String("])");
    return os.str();
}

LocalPositionUpdate::LocalPositionUpdate(const SpaceObjectReference& id, const Time t, const Vector3d& pos, const Quaternion& orient, const Vector3f& vel)
: Event(EventID(EventTypes::LocalPositionUpdate,id.toString())), time(t), position(pos), orientation(orient), velocity(vel), mID(id)
{
}

String LocalPositionUpdate::toString() const {
    std::ostringstream os;
    os << "(" << getId().toString() << " - [" << (time-Time::null()).toSeconds() << ", " <<
        position.toString() << ", " << orientation.toString() << ", " <<
        velocity.toString() << "])";
    return os.str();
}


EventID WorkQueueEvent::getID(int identifier) {
    return EventID(EventTypes::WorkQueue,boost::lexical_cast<String>(identifier));
}
WorkQueueEvent::WorkQueueEvent(int identifier):Event(getID(identifier)) {

}
QuitEvent::QuitEvent()
: Event(EventID(EventTypes::Quit))
{
}

NamedProxyCreated::NamedProxyCreated(const SpaceObjectReference& id, const String &_name) : Event(EventID(EventTypes::NamedProxyCreated,id.toString())), source(id), name(_name)
{
}

NamedProxyDeleted::NamedProxyDeleted(const SpaceObjectReference& id) : Event(EventID(EventTypes::NamedProxyDeleted,id.toString())),  source(id)
{
}

ProxySelected::ProxySelected(const SpaceObjectReference& id) : Event(EventID(EventTypes::ProxySelected,id.toString())), source(id)
{
}

ProxyDeselected::ProxyDeselected(const SpaceObjectReference& id) : Event(EventID(EventTypes::ProxyDeselected,id.toString())),  source(id)
{
}

} // namespace Meru

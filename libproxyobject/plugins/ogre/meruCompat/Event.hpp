/*  Meru
 *  Event.hpp
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
#ifndef _EVENT_HPP_
#define _EVENT_HPP_

#include "MeruDefs.hpp"
#include <task/Time.hpp>
#include <util/UUID.hpp>
#include <util/ObjectReference.hpp>
#include <util/SpaceObjectReference.hpp>
#include <vector>

namespace Meru {

typedef IdPair::Secondary EventSubType;
typedef IdPair::Primary EventType;
typedef IdPair EventID;

/*
class EventSubType {
public:
    EventSubType(int32_t val);
    ///For now uses the StringSubTypeImpl
    EventSubType(const UUID &val);
    ///For now uses the StringSubTypeImpl
    EventSubType(const SpaceObjectReference &val);
    ///For now uses the StringSubTypeImpl
    EventSubType(const SpaceID &val);
    ///For now uses the StringSubTypeImpl
    EventSubType(const ObjectReference &val);

    EventSubType(const String& val);
    EventSubType(const char* val);

    bool operator<(const EventSubType& rhs) const;
    bool operator==(const EventSubType& rhs) const;
    bool operator!=(const EventSubType& rhs) const;

    String toString() const;
private:
    void StringSubTypeImpl(const String&val);
    int32_t mIntVal;
    String mStringVal;
};
*/

namespace EventTypes {

extern const EventType WorkQueue;
extern const EventType Tick;
extern const EventType AvatarCreated;
extern const EventType ObjectDatabaseRestored;

extern const EventType ObjectCreated;
extern const EventType ObjectRestored;
extern const EventType ObjectDestroyed;

extern const EventType NamedProxyCreated;
extern const EventType NamedProxyDeleted;
extern const EventType PositionUpdate;
extern const EventType LocalPositionUpdate;
extern const EventType ProxySelected;
extern const EventType ProxyDeselected;
extern const EventType Quit;

} // namespace EventTypes


/** Identifier for types of events. Events can have types and subtypes.
 *  Often only the main type will be used.  A default subtype is provided
 *  and the default subtype will match all other subtypes (and can thus be
 *  used to ignore the subtype).
 */
/*
class EventID {
public:
    EventID(const EventType& etype = DefaultEventType, const EventSubType& estype = DefaultEventSubType);
    EventID(const EventID& cpy);

    static EventType registerEventType(const String& description);
    static const String& getEventTypeDescription(const EventType type);

    //EventID& operator=(const EventID& cpy);
    const EventType& type() const { return mType; }
    EventType& type() { return mType; }
    const EventSubType& subtype() const { return mSubType; }
    EventSubType& subtype() { return mSubType; }

    bool operator<(const EventID& rhs) const;
    bool operator==(const EventID& rhs) const;
    String toString()const{return String("[")+getEventTypeDescription(mType)+":"+mSubType.toString()+String("]");}
    const String& toStringEventType()const{return getEventTypeDescription(mType);}
    String toStringEventSubType()const{return mSubType.toString();}
private:
    EventType mType;
    EventSubType mSubType;
};
*/

/** Downcast an EventPtr to a shared pointer of the template type. */
template<typename T>
std::tr1::shared_ptr<T> DowncastEvent(const EventPtr& ptr) {
    return std::tr1::static_pointer_cast<T, Event>(ptr);
}

class TickEvent : public Event {
public:
    TickEvent(const Time& t, const Duration& dt);

    const Time& time() const { return mTime; }
    const Duration& timeSinceLastTick() const { return mDuration; }

    virtual String toString() const;
private:
    Time mTime;
    Duration mDuration;
};
typedef std::tr1::shared_ptr<TickEvent> TickEventPtr;

class UserEvent : public Event {
public:
    UserEvent(const EventID& evt_id, const SpaceObjectReference& id);

    const SpaceObjectReference& object() const { return mObject; }

    virtual String toString() const;
private:
    SpaceObjectReference mObject;
};
typedef std::tr1::shared_ptr<UserEvent> UserEventPtr;

class ObjectEvent : public Event {
public:
    ObjectEvent(const EventID& evt_id, const UUID& id);

    const UUID& uuid() const { return mUUID; }

    virtual String toString() const;
private:
    UUID mUUID;
};
typedef std::tr1::shared_ptr<ObjectEvent> ObjectEventPtr;

class ProximityEvent : public Event {
public:
    ProximityEvent(const EventID& evt_id, const ObjectReference& id1, const ObjectReference& id2);

    const ObjectReference* objects() const { return mObjects; }

    virtual String toString() const;
private:
    ObjectReference mObjects[2];
};
typedef std::tr1::shared_ptr<ProximityEvent> ProximityEventPtr;

class PositionUpdate : public Event {
public:
    PositionUpdate(const SpaceObjectReference& id, const Time t, const Vector3d& pos, const Quaternion& orient, const Vector3f& vel);

    const SpaceObjectReference object() const { return mID; }

    virtual String toString() const;

    Time time;
    Vector3d position;
    Quaternion orientation;
    Vector3f velocity;
private:
    SpaceObjectReference mID;
};
typedef std::tr1::shared_ptr<PositionUpdate> PositionUpdatePtr;

class LocalPositionUpdate : public Event {
public:
  LocalPositionUpdate(const SpaceObjectReference& id, const Time t, const Vector3d& pos, const Quaternion& orient, const Vector3f& vel);

  const SpaceObjectReference object() const { return mID; }

  virtual String toString() const;

  Time time;
  Vector3d position;
  Quaternion orientation;
  Vector3f velocity;
private:
  SpaceObjectReference mID;
};
typedef std::tr1::shared_ptr<LocalPositionUpdate> LocalPositionUpdatePtr;

class WorkQueueEvent:public Event {
public:
    WorkQueueEvent(int itemID);
    static EventID getID(int itemID);
};
class QuitEvent : public Event {
public:
	QuitEvent();
};
typedef std::tr1::shared_ptr<QuitEvent> QuitEventPtr;

class NamedProxyCreated : public Event {
public:
  NamedProxyCreated(const SpaceObjectReference& id, const String &name);
  SpaceObjectReference source;
  String name;
};
typedef std::tr1::shared_ptr<NamedProxyCreated> NamedProxyCreatedPtr;

class NamedProxyDeleted : public Event {
public:
  NamedProxyDeleted(const SpaceObjectReference& id);
  SpaceObjectReference source;
};
typedef std::tr1::shared_ptr<NamedProxyDeleted> NamedProxyDeletedPtr;

class ProxySelected : public Event {
public:
  ProxySelected(const SpaceObjectReference& id);
  SpaceObjectReference source;
};
typedef std::tr1::shared_ptr<ProxySelected> ProxySelectedPtr;

class ProxyDeselected : public Event {
public:
  ProxyDeselected(const SpaceObjectReference& id);
  SpaceObjectReference source;
};
typedef std::tr1::shared_ptr<ProxyDeselected> ProxyDeselectedPtr;

} // namespace Meru

#endif //_EVENT_HPP_

/*  cbr
 *  Message.hpp
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
 *  * Neither the name of cbr nor the names of its contributors may
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

#ifndef _CBR_MESSAGE_HPP_
#define _CBR_MESSAGE_HPP_

#include "Utility.hpp"
#include "Network.hpp"
#include "MotionVector.hpp"
#include "ServerNetwork.hpp"

namespace CBR {

typedef uint8 MessageType;

#define MESSAGE_TYPE_PROXIMITY     1
#define MESSAGE_TYPE_LOCATION      2
#define MESSAGE_TYPE_SUBSCRIPTION  3
#define MESSAGE_TYPE_MIGRATE       4
#define MESSAGE_TYPE_COUNT         5
#define MESSAGE_TYPE_CSEG_CHANGE   6
#define MESSAGE_TYPE_LOAD_STATUS   7
#define MESSAGE_TYPE_OSEG_MIGRATE  8
#define MESSAGE_TYPE_OSEG_LOOKUP   9
#define MESSAGE_TYPE_NOISE         10
#define MESSAGE_TYPE_SERVER_PROX_QUERY   11
#define MESSAGE_TYPE_SERVER_PROX_RESULT  12




struct OriginID {
    uint32 id;
};

template <typename scalar>
class SplitRegion {
public:
  ServerID mNewServerID;

  scalar minX, minY, minZ;
  scalar maxX, maxY, maxZ;

};

typedef SplitRegion<float> SplitRegionf;

typedef uint64 UniqueMessageID;
OriginID GetUniqueIDOriginID(UniqueMessageID uid);
uint64 GetUniqueIDMessageID(UniqueMessageID uid);

/** Base class for messages that go over the network.  Must provide
 *  message type and serialization methods.
 */
class Message {
public:
    virtual ~Message();

    virtual MessageType type() const = 0;
    UniqueMessageID id() const;

    virtual uint32 serialize(Network::Chunk& wire, uint32 offset) = 0;
    static uint32 deserialize(const Network::Chunk& wire, uint32 offset, Message** result);
protected:
    Message(const OriginID& origin, bool x); // note the bool is here to make the signature different than the next constructor
    Message(uint64 id);

    // takes care of serializing the header information properly, will overwrite
    // contents of chunk.  returns the offset after serializing the header
    uint32 serializeHeader(Network::Chunk& wire, uint32 offset);
private:
    static uint64 sIDSource;
    UniqueMessageID mID;
}; // class Message



/** Interface for classes that need to receive messages. */
class MessageRecipient {
public:
    virtual ~MessageRecipient() {}

    virtual void receiveMessage(Message* msg) = 0;
}; // class MessageRecipient

/** Base class for a message dispatcher. */
class MessageDispatcher {
public:
    void registerMessageRecipient(MessageType type, MessageRecipient* recipient);
    void unregisterMessageRecipient(MessageType type, MessageRecipient* recipient);

protected:
    void dispatchMessage(Message* msg) const;

private:
    typedef std::map<MessageType, MessageRecipient*> MessageRecipientMap;
    MessageRecipientMap mMessageRecipients;
}; // class MessageDispatcher

/** Base class for an object that can route messages to their destination. */
class MessageRouter {
public:
    virtual ~MessageRouter() {}

    virtual void route(Message* msg, const ServerID& dest_server, bool is_forward = false) = 0;
    virtual void route(Message* msg, const UUID& dest_obj, bool is_forward = false) = 0;
};

// Specific message types

class ProximityMessage : public Message {
public:
    enum EventType {
        Entered = 1,
        Exited = 2
    };

    ProximityMessage(const OriginID& origin, const UUID& dest_object, const UUID& nbr, EventType evt, const TimedMotionVector3f& loc);

    virtual MessageType type() const;

    const UUID& destObject() const;
    const UUID& neighbor() const;
    const EventType event() const;
    const TimedMotionVector3f& location() const;

    virtual uint32 serialize(Network::Chunk& wire, uint32 offset);
private:
    friend class Message;
    ProximityMessage(const Network::Chunk& wire, uint32& offset, uint64 _id);

    UUID mDestObject;
    UUID mNeighbor;
    EventType mEvent;
    TimedMotionVector3f mLocation;
}; // class ProximityMessage


// Base class for object to object messages.  Mostly saves a bit of serialization code
class ObjectToObjectMessage : public Message {
public:
    ObjectToObjectMessage(const OriginID& origin, const UUID& src_object, const UUID& dest_object);

    const UUID& sourceObject() const;
    const UUID& destObject() const;

protected:
    uint32 serializeSourceDest(Network::Chunk& wire, uint32 offset);
    ObjectToObjectMessage(const Network::Chunk& wire, uint32& offset, uint64 _id);

private:
    UUID mSourceObject;
    UUID mDestObject;
}; // class ObjectToObjectMessage


class LocationMessage : public ObjectToObjectMessage {
public:
    LocationMessage(const OriginID& origin, const UUID& src_object, const UUID& dest_object, const TimedMotionVector3f& loc);

    virtual MessageType type() const;

    const TimedMotionVector3f& location() const;

    virtual uint32 serialize(Network::Chunk& wire, uint32 offset);
private:
    friend class Message;
    LocationMessage(const Network::Chunk& wire, uint32& offset, uint64 _id);

    TimedMotionVector3f mLocation;
}; // class LocationMessage


class SubscriptionMessage : public ObjectToObjectMessage {
public:
    enum Action {
        Subscribe = 1,
        Unsubscribe = 2
    };

    SubscriptionMessage(const OriginID& origin, const UUID& src_object, const UUID& dest_object, const Action& act);

    virtual MessageType type() const;

    const Action& action() const;

    virtual uint32 serialize(Network::Chunk& wire, uint32 offset);
private:
    friend class Message;
    SubscriptionMessage(const Network::Chunk& wire, uint32& offset, uint64 _id);

    Action mAction;
}; // class SubscriptionMessage

class NoiseMessage : public Message {
public:
    NoiseMessage(const OriginID& origin, uint32 noise_sz);

    virtual MessageType type() const;

    virtual uint32 serialize(Network::Chunk& wire, uint32 offset);
private:
    friend class Message;
    NoiseMessage(const Network::Chunk& wire, uint32& offset, uint64 _id);

    uint32 mNoiseSize;
}; // class NoiseMessage

class MigrateMessage : public Message {
public:
    MigrateMessage(const OriginID& origin, const UUID& obj, SolidAngle queryAngle, uint16_t subscriberCount, ServerID from);

    ~MigrateMessage();

    virtual MessageType type() const;

    const UUID& object() const;

    const SolidAngle queryAngle() const;

    const int subscriberCount() const;

    UUID* subscriberList() const;

    virtual uint32 serialize(Network::Chunk& wire, uint32 offset);

    ServerID messageFrom();

private:
    friend class Message;
    MigrateMessage(const Network::Chunk& wire, uint32& offset, uint64 _id);


    UUID mObject;

    SolidAngle mQueryAngle;

    uint16_t mCountSubscribers;
    UUID* mSubscribers;
    ServerID mFrom;

}; // class MigrateMessage

class CSegChangeMessage : public Message {
public:
  CSegChangeMessage(const OriginID& origin, uint8_t number_of_regions);

  ~CSegChangeMessage();

  virtual MessageType type() const;

  const uint8_t numberOfRegions() const;

  SplitRegionf* splitRegions() const;

  virtual uint32 serialize(Network::Chunk& wire, uint32 offset);

private:
  friend class Message;
  CSegChangeMessage(const Network::Chunk& wire, uint32& offset, uint64 _id);


  uint8_t mNumberOfRegions;
  SplitRegionf* mSplitRegions;

};



  //oseg message


  class OSegMigrateMessage : public Message
  {
  public:
    enum OSegMigrateAction {CREATE,KILL,MOVE,ACKNOWLEDGE};

    OSegMigrateMessage(const OriginID& origin,ServerID sID_from, ServerID sID_to, ServerID sMessageDest, ServerID sMessageFrom, UUID obj_id, OSegMigrateAction action);
    ~OSegMigrateMessage();

    virtual MessageType type() const;
    virtual uint32 serialize(Network::Chunk& wire, uint32 offset);

    ServerID            getServFrom();
    ServerID            getServTo();
    UUID                getObjID();
    OSegMigrateAction   getAction();
    ServerID            getMessageDestination();
    ServerID            getMessageFrom();


  private:
    friend class Message;
    OSegMigrateMessage(const Network::Chunk& wire, uint32& offset, uint64 _id);

    ServerID mServID_from, mServID_to, mMessageDestination, mMessageFrom; //need from so know where to send acknowledge back to.
    UUID mObjID;
    OSegMigrateAction mAction;

  };


  class OSegLookupMessage : public Message
  {
  public:
    enum OSegLookupAction {I_HAVE_IT, WHERE_IS_IT};

    OSegLookupMessage(const OriginID& origin,ServerID sID_seeker, ServerID sID_keeper, UUID obj_id, OSegLookupAction action);
    ~OSegLookupMessage();

    virtual MessageType type() const;
    virtual uint32 serialize(Network::Chunk& wire, uint32 offset);

    ServerID getSeeker();
    ServerID getKeeper();
    UUID     getObjID();
    OSegLookupAction getAction();

  private:
    friend class Message;
    OSegLookupMessage(const Network::Chunk& wire, uint32& offset, uint64 _id);
    ServerID mSeeker,mKeeper; //seeker represents the server that is looking for the object.  keeper represents the server that has the object.
    UUID mObjID; //the id of the object that we are looking for.
    OSegLookupAction mAction; //are we posting that we're looking for something or that we have something?

  };



  //end oseg message



class LoadStatusMessage : public Message {

public:
  LoadStatusMessage(const OriginID& origin, float loadReading);

  ~LoadStatusMessage();

  virtual MessageType type() const;

  const float loadReading() const;

  virtual uint32 serialize(Network::Chunk& wire, uint32 offset);

private:
  friend class Message;
  LoadStatusMessage(const Network::Chunk& wire, uint32& offset, uint64 _id);


  float mLoadReading;

};



class ServerProximityQueryMessage : public Message {
public:
    enum QueryAction {
        AddOrUpdate = 1,
        Remove = 2
    };

    ServerProximityQueryMessage(const OriginID& origin, QueryAction action);
    ServerProximityQueryMessage(const OriginID& origin, QueryAction action, const TimedMotionVector3f& loc, const BoundingSphere3f& bounds, const SolidAngle& angle);
    ~ServerProximityQueryMessage();

    virtual MessageType type() const;
    virtual uint32 serialize(Network::Chunk& wire, uint32 offset);

    QueryAction action() const { return mAction; }
    TimedMotionVector3f queryLocation() const { assert(action() == AddOrUpdate); return mQueryLocation; }
    BoundingSphere3f queryBounds() const { assert(action() == AddOrUpdate); return mBounds; }
    SolidAngle queryAngle() const { assert(action() == AddOrUpdate); return mMinAngle; }
private:
    friend class Message;
    ServerProximityQueryMessage(const Network::Chunk& wire, uint32& offset, uint64 _id);

    QueryAction mAction;
    TimedMotionVector3f mQueryLocation;
    BoundingSphere3f mBounds;
    SolidAngle mMinAngle;
};


class ServerProximityResultMessage : public Message {
public:
    struct ObjectUpdate {
        UUID object;
        TimedMotionVector3f location;
        BoundingSphere3f bounds;
    };

    ServerProximityResultMessage(const OriginID& origin);
    ~ServerProximityResultMessage();

    virtual MessageType type() const;
    virtual uint32 serialize(Network::Chunk& wire, uint32 offset);

    void addObjectUpdate(const UUID& objid, const TimedMotionVector3f& loc, const BoundingSphere3f& bounds);
    void addObjectRemoval(const UUID& objid);

    std::vector<ObjectUpdate> objectUpdates() const {
        return mObjectUpdates;
    }

    std::vector<UUID> objectRemovals() const {
        return mObjectRemovals;
    }

private:
    friend class Message;
    ServerProximityResultMessage(const Network::Chunk& wire, uint32& offset, uint64 _id);

    std::vector<ObjectUpdate> mObjectUpdates;
    std::vector<UUID> mObjectRemovals;
};

} // namespace CBR

#endif //_CBR_MESSAGE_HPP_

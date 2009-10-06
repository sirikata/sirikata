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

#include "CBR_ServerMessage.pbj.hpp"
#include "CBR_ObjectMessage.pbj.hpp"
#include "CBR_Loc.pbj.hpp"
#include "CBR_Prox.pbj.hpp"
#include "CBR_Subscription.pbj.hpp"
#include "CBR_Migration.pbj.hpp"
#include "CBR_CSeg.pbj.hpp"
#include "CBR_Session.pbj.hpp"
#include "CBR_OSeg.pbj.hpp"
#include "CBR_ObjConnKill.pbj.hpp"


namespace CBR {

typedef uint8 MessageType;

#define MESSAGE_TYPE_OBJECT                     1
#define MESSAGE_TYPE_MIGRATE                    4
#define MESSAGE_TYPE_COUNT                      5
#define MESSAGE_TYPE_CSEG_CHANGE                6
#define MESSAGE_TYPE_LOAD_STATUS                7
#define MESSAGE_TYPE_OSEG_MIGRATE_MOVE          8
#define MESSAGE_TYPE_OSEG_MIGRATE_ACKNOWLEDGE   9
#define MESSAGE_TYPE_NOISE                     10
#define MESSAGE_TYPE_SERVER_PROX_QUERY         11
#define MESSAGE_TYPE_SERVER_PROX_RESULT        12
#define MESSAGE_TYPE_BULK_LOCATION             13
#define MESSAGE_TYPE_KILL_OBJ_CONN             14
#define MESSAGE_TYPE_UPDATE_OSEG               15
#define MESSAGE_TYPE_FORWARDED                 16
#define MESSAGE_TYPE_OSEG_ADDED_OBJECT         17
#define MESSAGE_TYPE_OBJECT_NOISE              18
#define MESSAGE_TYPE_UNPROCESSED_PACKET        255



// List of well known server ports, which should replace message types
#define SERVER_PORT_OBJECT_MESSAGE_ROUTING 1

// List of well known object ports, which should replace message types
#define OBJECT_PORT_SESSION       1
#define OBJECT_PORT_PROXIMITY     2
#define OBJECT_PORT_LOCATION      3
#define OBJECT_PORT_SUBSCRIPTION  4
#define OBJECT_PORT_NOISE 253
#define OBJECT_PORT_PING  254

template <typename scalar>
class SplitRegion {
public:
  ServerID mNewServerID;

  scalar minX, minY, minZ;
  scalar maxX, maxY, maxZ;

};

typedef SplitRegion<float> SplitRegionf;

typedef uint64 UniqueMessageID;
uint64 GenerateUniqueID(const ServerID& origin);
ServerID GetUniqueIDServerID(UniqueMessageID uid);
uint64 GetUniqueIDMessageID(UniqueMessageID uid);


template<typename PBJMessageType>
std::string serializePBJMessage(const PBJMessageType& contents) {
    std::string payload;
    bool serialized_success = contents.SerializeToString(&payload);
    assert(serialized_success);

    return payload;
}

template<typename PBJMessageType>
bool serializePBJMessage(std::string* payload, const PBJMessageType& contents) {
    return contents.SerializeToString(payload);
}

/** Parse a PBJ message from the wire, starting at the given offset from the .
 *  \param contents pointer to the message to parse from the data
 *  \param wire the raw data to parse from, either a Network::Chunk or std::string
 *  \param offset the number of bytes into the data to start parsing
 *  \param length the number of bytes to parse, or -1 to use the entire
 *  \returns true if parsed successfully, false otherwise
 */
template<typename PBJMessageType, typename WireType>
bool parsePBJMessage(PBJMessageType* contents, const WireType& wire, uint32 offset = 0, int32 length = -1) {
    assert(contents != NULL);
    uint32 rlen = (length == -1) ? (wire.size() - offset) : length;
    assert(offset + rlen <= wire.size()); // buffer overrun
    return contents->ParseFromArray((void*)&wire[offset], rlen);
}

// FIXME we're using a nasty custom framing for object host messages
std::string* serializeObjectHostMessage(const CBR::Protocol::Object::ObjectMessage& msg);

CBR::Protocol::Object::ObjectMessage* createObjectMessage(ServerID source_server, const UUID& src, uint16 src_port, const UUID& dest, uint16 dest_port, const std::string& payload);


/** Base class for messages that go over the network.  Must provide
 *  message type and serialization methods.
 */
class Message {
public:
    virtual ~Message();

    virtual MessageType type() const = 0;
    UniqueMessageID id() const;

    ServerID sourceServer() const { return mSource; }
    void setSourceServer(ServerID source) { mSource = source; }
    ServerID destServer() const { return mDest; }
    void setDestServer(ServerID dest) { mDest = dest; }

    virtual bool serialize(Network::Chunk* result) = 0;
    static Message* deserialize(const Network::Chunk& wire);

    virtual uint32 serializedSize() const = 0;
    uint32 size() const { return serializedSize(); }

protected:
    Message(const ServerID& origin);
    Message(const CBR::Protocol::Server::ServerMessage& svr_msg);

    template<typename ContentsPBJType>
    void fillMessage(CBR::Protocol::Server::ServerMessage& msg, const ContentsPBJType& pbj) const {
        msg.set_source_server( mSource );
        msg.set_source_port( this->type() );
        msg.set_dest_server( mDest );
        msg.set_dest_port( this->type() );
        msg.set_id(mID);

        msg.set_payload( serializePBJMessage(pbj) ); // FIXME should return false if this fails
    }

    template<typename ContentsPBJType>
    uint32 getSerializedSize(const ContentsPBJType& pbj) const {
        // FIXME having to serialize to find out the size is ridiculous, but there doesn't seem to be
        // an easy way to factor out the cost of the payload

        CBR::Protocol::Server::ServerMessage msg;
        fillMessage(msg, pbj);
        return msg.ByteSize();
    }

    template<typename ContentsPBJType>
    bool serializeContents(Network::Chunk* output, const ContentsPBJType& pbj) const {
        CBR::Protocol::Server::ServerMessage msg;
        fillMessage(msg, pbj);

        // FIXME having to copy here sucks
        std::string result;
        bool success = serializePBJMessage(&result, msg);
        if (!success) return false;
        output->resize( result.size() );
        memcpy(&((*output)[0]), &(result[0]), sizeof(uint8)*result.size());
        return true;
    }

private:
    UniqueMessageID mID;
    ServerID mSource;
    ServerID mDest;
}; // class Message



/** Interface for classes that need to receive messages. */
class MessageRecipient {
public:
    virtual ~MessageRecipient() {}

    virtual void receiveMessage(Message* msg) = 0;
}; // class MessageRecipient

/** Interface for classes that need to receive object messages, i.e. those that
 *  need to talk to objects/object hosts.
 */
class ObjectMessageRecipient {
public:
    virtual ~ObjectMessageRecipient() {}

    virtual void receiveMessage(const CBR::Protocol::Object::ObjectMessage& msg) = 0;
};

/** Base class for a message dispatcher. */
class MessageDispatcher {
public:
    void registerMessageRecipient(MessageType type, MessageRecipient* recipient);
    void unregisterMessageRecipient(MessageType type, MessageRecipient* recipient);

    // Registration and unregistration for object messages destined for the space
    void registerObjectMessageRecipient(uint16 port, ObjectMessageRecipient* recipient);
    void unregisterObjectMessageRecipient(uint16 port, ObjectMessageRecipient* recipient);
protected:
    virtual void dispatchMessage(Message* msg) const;
    virtual void dispatchMessage(const CBR::Protocol::Object::ObjectMessage& msg) const;

private:
    typedef std::map<MessageType, MessageRecipient*> MessageRecipientMap;
    MessageRecipientMap mMessageRecipients;
    typedef std::map<uint16, ObjectMessageRecipient*> ObjectMessageRecipientMap;
    ObjectMessageRecipientMap mObjectMessageRecipients;
}; // class MessageDispatcher

/** Base class for an object that can route messages to their destination. */
class MessageRouter {
public:
    enum SERVICES{
        PROXS,
        MIGRATES,
        LOCS,
        CSEGS,
        SERVER_TO_OBJECT_MESSAGESS,
        OSEG_TO_OBJECT_MESSAGESS,
        OBJECT_MESSAGESS,
        OSEG_CACHE_UPDATE,
        NUM_SERVICES
    };

    virtual ~MessageRouter() {}

    __attribute__ ((warn_unused_result))
    virtual bool route(SERVICES svc, Message* msg, bool is_forward = false) = 0;

    __attribute__ ((warn_unused_result))
  virtual bool route(CBR::Protocol::Object::ObjectMessage* msg, bool is_forward, ServerID forwardFrom = NullServerID) = 0;

};




// Specific message types

class ObjectMessage : public Message {
public:
    ObjectMessage(const ServerID& origin, const CBR::Protocol::Object::ObjectMessage& src);

    virtual MessageType type() const;
    virtual bool serialize(Network::Chunk* result);
    virtual uint32 serializedSize() const;

    CBR::Protocol::Object::ObjectMessage contents;
private:
    friend class Message;
    ObjectMessage(const CBR::Protocol::Server::ServerMessage& svr_msg);
}; // class ObjectMessage


class NoiseMessage : public Message {
public:
    NoiseMessage(const ServerID& origin, uint32 noise_sz);

    virtual MessageType type() const;
    virtual bool serialize(Network::Chunk* result);
    virtual uint32 serializedSize() const;

    CBR::Protocol::Object::Noise contents;
private:
    friend class Message;
    NoiseMessage(const CBR::Protocol::Server::ServerMessage& svr_msg);
}; // class NoiseMessage

class MigrateMessage : public Message {
public:
    MigrateMessage(const ServerID& origin);

    virtual MessageType type() const;
    virtual bool serialize(Network::Chunk* result);
    virtual uint32 serializedSize() const;

    CBR::Protocol::Migration::MigrationMessage contents;
private:
    friend class Message;
    MigrateMessage(const CBR::Protocol::Server::ServerMessage& svr_msg);
}; // class MigrateMessage

class CSegChangeMessage : public Message {
public:
    CSegChangeMessage(const ServerID& origin);

    virtual MessageType type() const;
    virtual bool serialize(Network::Chunk* result);
    virtual uint32 serializedSize() const;

    CBR::Protocol::CSeg::ChangeMessage contents;
private:
    friend class Message;
    CSegChangeMessage(const CBR::Protocol::Server::ServerMessage& svr_msg);
};

class LoadStatusMessage : public Message {
public:
    LoadStatusMessage(const ServerID& origin);

    virtual MessageType type() const;
    virtual bool serialize(Network::Chunk* result);
    virtual uint32 serializedSize() const;

    CBR::Protocol::CSeg::LoadMessage contents;
private:
    friend class Message;
    LoadStatusMessage(const CBR::Protocol::Server::ServerMessage& svr_msg);
};



  //oseg message

class OSegMigrateMessageMove : public Message
{
public:
  OSegMigrateMessageMove(const ServerID& origin,ServerID sID_from, ServerID sID_to, ServerID sMessageDest, ServerID sMessageFrom, UUID obj_id);


    virtual MessageType type() const;
    virtual bool serialize(Network::Chunk* result);
    virtual uint32 serializedSize() const;

    CBR::Protocol::OSeg::MigrateMessageMove contents;

    ServerID            getServFrom();
    ServerID            getServTo();
    UUID                getObjID();
    ServerID            getMessageDestination();
    ServerID            getMessageFrom();


private:
    friend class Message;
    OSegMigrateMessageMove(const CBR::Protocol::Server::ServerMessage& svr_msg);
};


class OSegMigrateMessageAcknowledge : public Message
{
public:
  OSegMigrateMessageAcknowledge(const ServerID& origin, const ServerID &sID_from, const ServerID &sID_to, const ServerID &sMessageDest, const ServerID &sMessageFrom, const UUID &obj_id);

  virtual MessageType type() const;
    virtual bool serialize(Network::Chunk* result);
    virtual uint32 serializedSize() const;

  CBR::Protocol::OSeg::MigrateMessageAcknowledge contents;

  ServerID            getServFrom();
  ServerID            getServTo();
  UUID                getObjID();
  ServerID            getMessageDestination();
  ServerID            getMessageFrom();


private:
  friend class Message;
  OSegMigrateMessageAcknowledge(const CBR::Protocol::Server::ServerMessage& svr_msg);
};


//update oseg message
class UpdateOSegMessage : public Message
{
public:
  UpdateOSegMessage(const ServerID& sID_sendingMessage, const ServerID& sID_objOn, const UUID& obj_id);
  virtual MessageType type() const;
    virtual bool serialize(Network::Chunk* result);
    virtual uint32 serializedSize() const;

  ~UpdateOSegMessage();

  CBR::Protocol::OSeg::UpdateOSegMessage contents;

private:
  friend class Message;
  UpdateOSegMessage(const CBR::Protocol::Server::ServerMessage& svr_msg);

};


  //end oseg message

//kill obj conn message
class KillObjConnMessage : public Message
{
public:
  CBR::Protocol::ObjConnKill::ObjConnKill contents;
  KillObjConnMessage(const ServerID& origin);
  ~KillObjConnMessage();
  virtual MessageType type() const;
    virtual bool serialize(Network::Chunk* result);
    virtual uint32 serializedSize() const;

private:
  friend class Message;
  KillObjConnMessage(const CBR::Protocol::Server::ServerMessage& svr_msg);
};

//end kill obj conn message


class OSegAddMessage : public Message
{
public:
  CBR::Protocol::OSeg::AddedObjectMessage contents;

  OSegAddMessage(const ServerID& origin, const UUID& obj_id);
  ~OSegAddMessage();
  virtual MessageType type() const;
    virtual bool serialize(Network::Chunk* result);
    virtual uint32 serializedSize() const;

private:
  friend class Message;
  OSegAddMessage(const CBR::Protocol::Server::ServerMessage& svr_msg);
};


//forwarded message
//class

//end


class ServerProximityQueryMessage : public Message {
public:
    ServerProximityQueryMessage(const ServerID& origin);
    ~ServerProximityQueryMessage();

    virtual MessageType type() const;
    virtual bool serialize(Network::Chunk* result);
    virtual uint32 serializedSize() const;

    CBR::Protocol::Prox::ServerQuery contents;
private:
    friend class Message;
    ServerProximityQueryMessage(const CBR::Protocol::Server::ServerMessage& svr_msg);
};


class ServerProximityResultMessage : public Message {
public:
    ServerProximityResultMessage(const ServerID& origin);
    ~ServerProximityResultMessage();

    virtual MessageType type() const;
    virtual bool serialize(Network::Chunk* result);
    virtual uint32 serializedSize() const;

    CBR::Protocol::Prox::ProximityResults contents;
private:
    friend class Message;
    ServerProximityResultMessage(const CBR::Protocol::Server::ServerMessage& svr_msg);
};


/** Location updates from Prox/Loc to an object. */
class BulkLocationMessage : public Message {
public:
    BulkLocationMessage(const ServerID& origin);
    ~BulkLocationMessage();

    virtual MessageType type() const;
    virtual bool serialize(Network::Chunk* result);
    virtual uint32 serializedSize() const;

    CBR::Protocol::Loc::BulkLocationUpdate contents;
private:
    friend class Message;
    BulkLocationMessage(const CBR::Protocol::Server::ServerMessage& svr_msg);
};

} // namespace CBR

#endif //_CBR_MESSAGE_HPP_

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

#include "CBR_ServerMessage.pbj.hpp"
#include "CBR_ObjectMessage.pbj.hpp"
#include "CBR_SSTHeader.pbj.hpp"


namespace CBR {

typedef uint16 ServerMessagePort;

// List of well known server ports.
// FIXME Reduce the number of these ports (combine related ones), reorder, and renumber
#define SERVER_PORT_OBJECT_MESSAGE_ROUTING     1
#define SERVER_PORT_LOCATION                   13
#define SERVER_PORT_PROX                       11
#define SERVER_PORT_MIGRATION                  4
#define SERVER_PORT_KILL_OBJ_CONN              14
#define SERVER_PORT_CSEG_CHANGE                6
#define SERVER_PORT_LOAD_STATUS                7
#define SERVER_PORT_OSEG_ADDED_OBJECT          17
#define SERVER_PORT_OSEG_MIGRATE_MOVE          8
#define SERVER_PORT_OSEG_MIGRATE_ACKNOWLEDGE   9
#define SERVER_PORT_OSEG_UPDATE                15
#define SERVER_PORT_UNPROCESSED_PACKET         0xFFFF


typedef uint16 ObjectMessagePort;

// List of well known object ports.
#define OBJECT_PORT_SESSION       1
#define OBJECT_PORT_PROXIMITY     2
#define OBJECT_PORT_LOCATION      3
#define OBJECT_PORT_PING          254


typedef uint64 UniqueMessageID;

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

CBR::Protocol::Object::ObjectMessage* createObjectMessage(ServerID source_server, const UUID& src, uint16 src_port, const UUID& dest, uint16 dest_port, const std::string& payload);

/** Base class for messages that go over the network.  Must provide
 *  message type and serialization methods.
 */
class Message {
public:
    Message(const ServerID& origin);
    Message(ServerID src, uint16 src_port, ServerID dest, ServerID dest_port);
    Message(ServerID src, uint16 src_port, ServerID dest, uint16 dest_port, const std::string& pl);

    ServerID source_server() const { return mImpl.source_server(); }
    void set_source_server(const ServerID sid);

    uint16 source_port() const { return mImpl.source_port(); }
    void set_source_port(const uint16 port) { mImpl.set_source_port(port); }

    ServerID dest_server() const { return mImpl.dest_server(); }
    void set_dest_server(const ServerID sid) { mImpl.set_dest_server(sid); }

    uint16 dest_port() const { return mImpl.dest_port(); }
    void set_dest_port(const uint16 port) { mImpl.set_dest_port(port); }

    UniqueMessageID id() const { return mImpl.id(); }
    // NOTE: We don't expose set_id() so we can guarantee they will be unique

    std::string payload() const { return mImpl.payload(); }
    void set_payload(const std::string& pl) { mImpl.set_payload(pl); }


    bool ParseFromString(const std::string& data) {
        return mImpl.ParseFromString(data);
    }
    bool ParseFromArray(const void* data, int size) {
        return mImpl.ParseFromArray(data, size);
    }

    // Deprecated. Remains for backwards compatibility.
    bool serialize(Network::Chunk* result);
    static Message* deserialize(const Network::Chunk& wire);

    // Deprecated. Remains for backwards compatibility.
    uint32 serializedSize() const;
    uint32 size() const { return serializedSize(); }

protected:
    // Note: Should only be used for deserialization to ensure unique ID's are handled properly
    Message();

    void set_id(const UniqueMessageID _id) { mImpl.set_id(_id); }
private:
    CBR::Protocol::Server::ServerMessage mImpl;
}; // class Message




/** Interface for classes that need to receive messages. */
class MessageRecipient {
public:
    virtual ~MessageRecipient() {}

    virtual void receiveMessage(Message* msg) = 0;
}; // class MessageRecipient


// Wrapper class for Protocol::Object::Message which provides it some missing methods
// that are useful, e.g. size().
class ObjectMessage : public CBR::Protocol::Object::ObjectMessage {
public:
    uint32 size() {
        return this->ByteSize();
    }

    std::string* serialize() {
        return new std::string( serializePBJMessage(*this) );
    };
}; // class ObjectMessage

// FIXME get rid of this
ObjectMessage* createObjectHostMessage(ObjectHostID source_server, const UUID& src, uint16 src_port, const UUID& dest, uint16 dest_port, const std::string& payload);


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
    void registerMessageRecipient(ServerMessagePort type, MessageRecipient* recipient);
    void unregisterMessageRecipient(ServerMessagePort type, MessageRecipient* recipient);

    // Registration and unregistration for object messages destined for the space
    void registerObjectMessageRecipient(ObjectMessagePort port, ObjectMessageRecipient* recipient);
    void unregisterObjectMessageRecipient(ObjectMessagePort port, ObjectMessageRecipient* recipient);
protected:
    virtual void dispatchMessage(Message* msg) const;
    virtual void dispatchMessage(const CBR::Protocol::Object::ObjectMessage& msg) const;

private:
    typedef std::map<ServerMessagePort, MessageRecipient*> MessageRecipientMap;
    MessageRecipientMap mMessageRecipients;
    typedef std::map<ObjectMessagePort, ObjectMessageRecipient*> ObjectMessageRecipientMap;
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
        OSEG_TO_OBJECT_MESSAGESS,
        OBJECT_MESSAGESS,
        OSEG_CACHE_UPDATE,
        NUM_SERVICES
    };

    virtual ~MessageRouter() {}

    WARN_UNUSED
    virtual bool route(SERVICES svc, Message* msg) = 0;

    WARN_UNUSED
    virtual bool route(CBR::Protocol::Object::ObjectMessage* msg) = 0;
};


} // namespace CBR

#endif //_CBR_MESSAGE_HPP_

/*  Sirikata
 *  ServerMessage.hpp
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

#ifndef _SIRIKATA_SERVER_MESSAGE_HPP_
#define _SIRIKATA_SERVER_MESSAGE_HPP_

#include <sirikata/core/util/Platform.hpp>

#include <sirikata/core/network/Message.hpp>
#include <sirikata/core/network/ObjectMessage.hpp>

#include "CBR_ServerMessage.pbj.hpp"
//#include "CBR_SSTHeader.pbj.hpp"


namespace Sirikata {

typedef uint16 ServerMessagePort;

// List of well known server ports.
// FIXME Reduce the number of these ports (combine related ones), reorder, and renumber
#define SERVER_PORT_OBJECT_MESSAGE_ROUTING     1
#define SERVER_PORT_LOCATION                   13
#define SERVER_PORT_PROX                       11
#define SERVER_PORT_MIGRATION                  4
#define SERVER_PORT_CSEG_CHANGE                6
#define SERVER_PORT_LOAD_STATUS                7
#define SERVER_PORT_OSEG_MIGRATE_MOVE          8
#define SERVER_PORT_OSEG_MIGRATE_ACKNOWLEDGE   9
#define SERVER_PORT_OSEG_UPDATE                15
#define SERVER_PORT_FORWARDER_WEIGHT_UPDATE    16
#define SERVER_PORT_UNPROCESSED_PACKET         0xFFFF

/** Base class for messages that go over the network.  Must provide
 *  message type and serialization methods.
 */
class Message {
public:
    Message(const ServerID& origin);
    Message(ServerID src, uint16 src_port, ServerID dest, ServerID dest_port);
    Message(ServerID src, uint16 src_port, ServerID dest, uint16 dest_port, const std::string& pl);
    Message(ServerID src, uint16 src_port, ServerID dest, uint16 dest_port, const Sirikata::Protocol::Object::ObjectMessage* pl);

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

    UniqueMessageID payload_id() const { return mImpl.payload_id(); }
    // NOTE: We don't expose set_id() so we guarantee it gets created properly.
    // Use the constructor taking an ObjectMessage to ensure this works properly.

    std::string payload() const { return mImpl.payload(); }
    void set_payload(const std::string& pl) { mImpl.set_payload(pl); }


    bool ParseFromString(const std::string& data) {
        return mImpl.ParseFromString(data);
    }
    bool ParseFromArray(const void* data, int size) {
        return mImpl.ParseFromArray(data, size);
    }

    // Deprecated. Remains for backwards compatibility.
    bool serialize(Network::Chunk* result) const;
    static Message* deserialize(const Network::Chunk& wire);

    // Deprecated. Remains for backwards compatibility.
    uint32 serializedSize() const;
    uint32 size() const { return serializedSize(); }

protected:
    // Note: Should only be used for deserialization to ensure unique ID's are handled properly
    Message();

    void set_id(const UniqueMessageID _id) { mImpl.set_id(_id); }
    void set_payload_id(const UniqueMessageID _id) { mImpl.set_payload_id(_id); }
private:
    // Helper methods to fill in message data
    void fillMessage(ServerID src, uint16 src_port, ServerID dest, uint16 dest_port);
    void fillMessage(ServerID src, uint16 src_port, ServerID dest, ServerID dest_port, const std::string& pl);

    Sirikata::Protocol::Server::ServerMessage mImpl;
    mutable uint32 mCachedSize;
}; // class Message




/** Interface for classes that need to receive messages. */
class MessageRecipient {
public:
    virtual ~MessageRecipient() {}

    virtual void receiveMessage(Message* msg) = 0;
}; // class MessageRecipient


/** Base class for a message dispatcher. */
class ServerMessageDispatcher {
public:
    virtual ~ServerMessageDispatcher() {}

    void registerMessageRecipient(ServerMessagePort type, MessageRecipient* recipient);
    void unregisterMessageRecipient(ServerMessagePort type, MessageRecipient* recipient);


protected:
    virtual void dispatchMessage(Message* msg) const;


private:
    typedef std::map<ServerMessagePort, MessageRecipient*> MessageRecipientMap;
    MessageRecipientMap mMessageRecipients;

}; // class ServerMessageDispatcher

template<typename MessageType>
class Router {
  public:
    virtual ~Router() {}
    WARN_UNUSED
    virtual bool route(MessageType msg) = 0;
}; // class Router

/** Base class for an object that can route messages to their destination. */
class ServerMessageRouter {
public:
    virtual ~ServerMessageRouter() {}

    virtual Router<Message*>* createServerMessageService(const String& name) = 0;
};

} // namespace Sirikata

#endif //_SIRIKATA_SERVER_MESSAGE_HPP_

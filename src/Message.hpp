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

#include "Utility.hpp"
#include "Network.hpp"
#include "Server.hpp"

namespace CBR {

typedef uint8 MessageType;

#define MESSAGE_TYPE_PROXIMITY    1
#define MESSAGE_TYPE_LOCATION     2
#define MESSAGE_TYPE_MIGRATE      3

/** Base class for messages that go over the network.  Must provide source,
 *  destination, message type, and serialization methods.
 */
class Message {
public:
    Message(const ServerID& src_server, const ServerID& dst_server, MessageType t);
    virtual ~Message();

    const ServerID& sourceServer() const;
    const ServerID& destServer() const;
    MessageType type() const;

    virtual Network::Chunk serialize() = 0;
    static Message* deserialize(const Network::Chunk& wire);
protected:
    // takes care of serializing the header information properly, will overwrite
    // contents of chunk.  returns the offset after serializing the header
    uint32 serializeHeader(Network::Chunk& wire);
private:
    Message();

    ServerID mSourceServer;
    ServerID mDestServer;
    MessageType mType;
}; // class Message


class ProximityMessage : public Message {
public:
    enum EventType {
        Entered = 1,
        Exited = 2
    };

    ProximityMessage(const ServerID& src_server, const ServerID& dst_server, const UUID& dest_object, const UUID& nbr, EventType evt);

    const UUID& destObject() const;
    const UUID& neighbor() const;
    const EventType event() const;

    virtual Network::Chunk serialize();
private:
    friend class Message;
    ProximityMessage(const ServerID& src_server, const ServerID& dst_server, const Network::Chunk& wire, uint32 offset);

    UUID mDestObject;
    UUID mNeighbor;
    EventType mEvent;
}; // class ProximityMessage



class LocationMessage : public Message {
public:
    LocationMessage(const ServerID& src_server, const ServerID& dst_server, const UUID& src_object, const UUID& dest_object, const MotionVector3f& loc);

    const UUID& sourceObject() const;
    const UUID& destObject() const;
    const MotionVector3f& location() const;

    virtual Network::Chunk serialize();
private:
    friend class Message;
    LocationMessage(const ServerID& src_server, const ServerID& dst_server, const Network::Chunk& wire, uint32 offset);

    UUID mSourceObject;
    UUID mDestObject;
    MotionVector3f mLocation;
}; // class LocationMessage



class MigrateMessage : public Message {
public:
    MigrateMessage(const ServerID& src_server, const ServerID& dst_server, const UUID& obj);

    const UUID& object() const;

    virtual Network::Chunk serialize();
private:
    friend class Message;
    MigrateMessage(const ServerID& src_server, const ServerID& dst_server, const Network::Chunk& wire, uint32 offset);

    UUID mObject;
}; // class MigrateMessage

} // namespace CBR

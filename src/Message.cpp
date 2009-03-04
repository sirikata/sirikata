/*  cbr
 *  Message.cpp
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

#include "Message.hpp"

namespace CBR {


ServerMessageHeader::ServerMessageHeader(const ServerID& src_server, const ServerID& dest_server)
 : mSourceServer(src_server),
   mDestServer(dest_server)
{
}

const ServerID& ServerMessageHeader::sourceServer() const {
    return mSourceServer;
}

const ServerID& ServerMessageHeader::destServer() const {
    return mDestServer;
}

uint32 ServerMessageHeader::serialize(Network::Chunk& wire, uint32 offset) {
    if (wire.size() < offset + 2 * sizeof(ServerID) )
        wire.resize( offset + 2 * sizeof(ServerID) );

    memcpy( &wire[offset], &mSourceServer, sizeof(ServerID) );
    offset += sizeof(ServerID);

    memcpy( &wire[offset], &mDestServer, sizeof(ServerID) );
    offset += sizeof(ServerID);

    return offset;
}

ServerMessageHeader ServerMessageHeader::deserialize(const Network::Chunk& wire, uint32 offset) {
    ServerID raw_source;
    ServerID raw_dest;

    memcpy( &raw_source, &wire[offset], sizeof(ServerID) );
    offset += sizeof(ServerID);

    memcpy( &raw_dest, &wire[offset], sizeof(ServerID) );
    offset += sizeof(ServerID);

    return ServerMessageHeader(raw_source, raw_dest);
}




Message::Message(MessageType t)
 : mType(t)
{
    assert( t > 0 && t < MESSAGE_TYPE_COUNT );
}

Message::~Message() {
}

MessageType Message::type() const {
    return mType;
}

uint32 Message::deserialize(const Network::Chunk& wire, uint32 offset, Message** result) {
    uint8 raw_type;

    memcpy( &raw_type, &wire[offset], 1 );
    offset += 1;

    Message* msg = NULL;

    switch(raw_type) {
      case MESSAGE_TYPE_PROXIMITY:
        msg = new ProximityMessage(wire, offset);
        break;
      case MESSAGE_TYPE_LOCATION:
        msg = new LocationMessage(wire, offset);
        break;
      case MESSAGE_TYPE_MIGRATE:
        msg = new MigrateMessage(wire, offset);
        break;
      default:
#if NDEBUG
        assert(false);
#endif
        break;
    }

    if (result != NULL)
        *result = msg;
    else // if they didn't want the result, just delete it
        delete msg;

    return offset;
}

uint32 Message::serializeHeader(Network::Chunk& wire, uint32 offset) {
    if (wire.size() < offset + 1)
        wire.resize( offset + 1 );

    memcpy( &wire[offset], &mType, 1 );
    offset += 1;

    return offset;
}



ProximityMessage::ProximityMessage(const UUID& dst_object, const UUID& nbr, EventType evt)
 : Message(MESSAGE_TYPE_PROXIMITY),
   mDestObject(dst_object),
   mNeighbor(nbr),
   mEvent(evt)
{
}

ProximityMessage::ProximityMessage(const Network::Chunk& wire, uint32& offset)
 : Message(MESSAGE_TYPE_PROXIMITY)
{
    uint8 raw_dest_object[UUID::static_size];
    uint8 raw_neighbor[UUID::static_size];
    uint8 raw_evt_type;

    memcpy( &raw_dest_object, &wire[offset], UUID::static_size );
    offset += UUID::static_size;
    mDestObject = UUID(raw_dest_object, UUID::static_size);

    memcpy( &raw_neighbor, &wire[offset], UUID::static_size );
    offset += UUID::static_size;
    mNeighbor = UUID(raw_neighbor, UUID::static_size);

    memcpy( &raw_evt_type, &wire[offset], 1 );
    offset += 1;
    mEvent = (EventType)raw_evt_type;
}

const UUID& ProximityMessage::destObject() const {
    return mDestObject;
}

const UUID& ProximityMessage::neighbor() const {
    return mNeighbor;
}

const ProximityMessage::EventType ProximityMessage::event() const {
    return mEvent;
}

uint32 ProximityMessage::serialize(Network::Chunk& wire, uint32 offset) {
    offset = serializeHeader(wire, offset);

    uint32 prox_part_size = 2 * UUID::static_size + 1;
    wire.resize( wire.size() + prox_part_size );

    memcpy( &wire[offset], mDestObject.getArray().data(), UUID::static_size );
    offset += UUID::static_size;

    memcpy( &wire[offset], mNeighbor.getArray().data(), UUID::static_size );
    offset += UUID::static_size;

    uint8 evt_type = (uint8)mEvent;
    memcpy( &wire[offset], &evt_type, 1 );
    offset += 1;

    return offset;
}



ObjectToObjectMessage::ObjectToObjectMessage(MessageType t, const UUID& src_object, const UUID& dest_object)
 : Message(t),
   mSourceObject(src_object),
   mDestObject(dest_object)
{
}

ObjectToObjectMessage::ObjectToObjectMessage(MessageType t, const Network::Chunk& wire, uint32& offset)
 : Message(t)
{
    uint8 raw_src_object[UUID::static_size];
    uint8 raw_dest_object[UUID::static_size];

    memcpy( &raw_src_object, &wire[offset], UUID::static_size );
    offset += UUID::static_size;
    mSourceObject = UUID(raw_src_object, UUID::static_size);

    memcpy( &raw_dest_object, &wire[offset], UUID::static_size );
    offset += UUID::static_size;
    mDestObject = UUID(raw_dest_object, UUID::static_size);
}

const UUID& ObjectToObjectMessage::sourceObject() const {
    return mSourceObject;
}

const UUID& ObjectToObjectMessage::destObject() const {
    return mDestObject;
}

uint32 ObjectToObjectMessage::serializeSourceDest(Network::Chunk& wire, uint32 offset) {
    uint32 loc_part_size = 2 * UUID::static_size;
    wire.resize( wire.size() + loc_part_size );

    memcpy( &wire[offset], mSourceObject.getArray().data(), UUID::static_size );
    offset += UUID::static_size;

    memcpy( &wire[offset], mDestObject.getArray().data(), UUID::static_size );
    offset += UUID::static_size;

    return offset;
}




LocationMessage::LocationMessage(const UUID& src_object, const UUID& dest_object, const MotionVector3f& loc)
 : ObjectToObjectMessage(MESSAGE_TYPE_LOCATION, src_object, dest_object),
   mLocation(loc)
{
}

LocationMessage::LocationMessage(const Network::Chunk& wire, uint32& offset)
 : ObjectToObjectMessage(MESSAGE_TYPE_LOCATION, wire, offset)
{
    memcpy( &mLocation, &wire[offset], sizeof(MotionVector3f) );
    offset += sizeof(MotionVector3f);
}

const MotionVector3f& LocationMessage::location() const {
    return mLocation;
}

uint32 LocationMessage::serialize(Network::Chunk& wire, uint32 offset) {
    offset = serializeHeader(wire, offset);
    offset = serializeSourceDest(wire, offset);

    uint32 loc_part_size = sizeof(MotionVector3f);
    wire.resize( wire.size() + loc_part_size );

    memcpy( &wire[offset], &mLocation, sizeof(MotionVector3f) );
    offset += sizeof(MotionVector3f);

    return offset;
}




MigrateMessage::MigrateMessage(const UUID& obj)
 : Message(MESSAGE_TYPE_MIGRATE),
   mObject(obj)
{
}

MigrateMessage::MigrateMessage(const Network::Chunk& wire, uint32& offset)
 : Message(MESSAGE_TYPE_MIGRATE)
{
    uint8 raw_object[UUID::static_size];

    memcpy( &raw_object, &wire[offset], UUID::static_size );
    offset += UUID::static_size;
    mObject = UUID(raw_object, UUID::static_size);
}


const UUID& MigrateMessage::object() const {
    return mObject;
}

uint32 MigrateMessage::serialize(Network::Chunk& wire, uint32 offset) {
    offset = serializeHeader(wire, offset);

    uint32 loc_part_size = UUID::static_size;
    wire.resize( wire.size() + loc_part_size );

    memcpy( &wire[offset], mObject.getArray().data(), UUID::static_size );
    offset += UUID::static_size;

    return offset;
}



} // namespace CBR

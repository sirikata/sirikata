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

Message::Message(const ServerID& src_server, const ServerID& dst_server, MessageType t)
 : mSourceServer(src_server), mDestServer(dst_server), mType(t)
{
}

Message::~Message() {
}

const ServerID& Message::sourceServer() const {
    return mSourceServer;
}

const ServerID& Message::destServer() const {
    return mDestServer;
}

MessageType Message::type() const {
    return mType;
}

Message* Message::deserialize(const Network::Chunk& wire) {
    ServerID raw_source;
    ServerID raw_dest;
    uint8 raw_type;

    uint32 offset = 0;

    memcpy( &raw_source, &wire[offset], sizeof(ServerID) );
    offset += sizeof(ServerID);

    memcpy( &raw_dest, &wire[offset], sizeof(ServerID) );
    offset += sizeof(ServerID);

    memcpy( &raw_type, &wire[offset], 1 );
    offset += 1;

    Message* msg = NULL;

    switch(raw_type) {
      case MESSAGE_TYPE_PROXIMITY:
        msg = new ProximityMessage(raw_source, raw_dest, wire, offset);
        break;
      case MESSAGE_TYPE_LOCATION:
        msg = new LocationMessage(raw_source, raw_dest, wire, offset);
        break;
      case MESSAGE_TYPE_MIGRATE:
        msg = new MigrateMessage(raw_source, raw_dest, wire, offset);
        break;
      default:
#if NDEBUG
        assert(false);
#endif
        break;
    }

    return msg;
}

uint32 Message::serializeHeader(Network::Chunk& wire) {
    if (wire.size() < 2 * sizeof(ServerID) + 1)
        wire.resize( 2 * sizeof(ServerID) + 1 );

    uint32 offset = 0;

    memcpy( &wire[offset], &mSourceServer, sizeof(ServerID) );
    offset += sizeof(ServerID);

    memcpy( &wire[offset], &mDestServer, sizeof(ServerID) );
    offset += sizeof(ServerID);

    memcpy( &wire[offset], &mType, 1 );
    offset += 1;

    return offset;
}



ProximityMessage::ProximityMessage(const ServerID& src_server, const ServerID& dst_server, const UUID& dst_object, const UUID& nbr, EventType evt)
 : Message(src_server, dst_server, MESSAGE_TYPE_PROXIMITY),
   mDestObject(dst_object),
   mNeighbor(nbr),
   mEvent(evt)
{
}

ProximityMessage::ProximityMessage(const ServerID& src_server, const ServerID& dst_server, const Network::Chunk& wire, uint32 offset)
 : Message(src_server, dst_server, MESSAGE_TYPE_PROXIMITY)
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

Network::Chunk ProximityMessage::serialize() {
    Network::Chunk result;

    uint32 offset = serializeHeader(result);

    uint32 prox_part_size = 2 * UUID::static_size + 1;
    result.resize( result.size() + prox_part_size );

    memcpy( &result[offset], mDestObject.getArray().data(), UUID::static_size );
    offset += UUID::static_size;

    memcpy( &result[offset], mNeighbor.getArray().data(), UUID::static_size );
    offset += UUID::static_size;

    uint8 evt_type = (uint8)mEvent;
    memcpy( &result[offset], &evt_type, 1 );
    offset += 1;

    return result;
}




LocationMessage::LocationMessage(const ServerID& src_server, const ServerID& dst_server, const UUID& src_object, const UUID& dest_object, const MotionVector3f& loc)
 : Message(src_server, dst_server, MESSAGE_TYPE_LOCATION),
   mSourceObject(src_object),
   mDestObject(dest_object),
   mLocation(loc)
{
}

LocationMessage::LocationMessage(const ServerID& src_server, const ServerID& dst_server, const Network::Chunk& wire, uint32 offset)
 : Message(src_server, dst_server, MESSAGE_TYPE_LOCATION)
{
    uint8 raw_src_object[UUID::static_size];
    uint8 raw_dest_object[UUID::static_size];

    memcpy( &raw_src_object, &wire[offset], UUID::static_size );
    offset += UUID::static_size;
    mSourceObject = UUID(raw_src_object, UUID::static_size);

    memcpy( &raw_dest_object, &wire[offset], UUID::static_size );
    offset += UUID::static_size;
    mDestObject = UUID(raw_dest_object, UUID::static_size);

    memcpy( &mLocation, &wire[offset], sizeof(MotionVector3f) );
    offset += sizeof(MotionVector3f);
}

const UUID& LocationMessage::sourceObject() const {
    return mSourceObject;
}

const UUID& LocationMessage::destObject() const {
    return mDestObject;
}

const MotionVector3f& LocationMessage::location() const {
    return mLocation;
}

Network::Chunk LocationMessage::serialize() {
    Network::Chunk result;

    uint32 offset = serializeHeader(result);

    uint32 loc_part_size = 2 * UUID::static_size + sizeof(MotionVector3f);
    result.resize( result.size() + loc_part_size );

    memcpy( &result[offset], mSourceObject.getArray().data(), UUID::static_size );
    offset += UUID::static_size;

    memcpy( &result[offset], mDestObject.getArray().data(), UUID::static_size );
    offset += UUID::static_size;

    memcpy( &result[offset], &mLocation, sizeof(MotionVector3f) );
    offset += sizeof(MotionVector3f);

    return result;
}




MigrateMessage::MigrateMessage(const ServerID& src_server, const ServerID& dst_server, const UUID& obj)
 : Message(src_server, dst_server, MESSAGE_TYPE_MIGRATE),
   mObject(obj)
{
}

MigrateMessage::MigrateMessage(const ServerID& src_server, const ServerID& dst_server, const Network::Chunk& wire, uint32 offset)
 : Message(src_server, dst_server, MESSAGE_TYPE_MIGRATE)
{
    uint8 raw_object[UUID::static_size];

    memcpy( &raw_object, &wire[offset], UUID::static_size );
    offset += UUID::static_size;
    mObject = UUID(raw_object, UUID::static_size);
}


const UUID& MigrateMessage::object() const {
    return mObject;
}

Network::Chunk MigrateMessage::serialize() {
    Network::Chunk result;

    uint32 offset = serializeHeader(result);

    uint32 loc_part_size = UUID::static_size;
    result.resize( result.size() + loc_part_size );

    memcpy( &result[offset], mObject.getArray().data(), UUID::static_size );
    offset += UUID::static_size;

    return result;
}



} // namespace CBR

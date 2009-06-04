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

#define MESSAGE_ID_SERVER_SHIFT 20
#define MESSAGE_ID_SERVER_BITS 0xFFF00000

static uint32 GenerateUniqueID(const OriginID& origin, uint32 id_src) {
    uint32 server_int = (uint32)origin.id;
    uint32 server_shifted = server_int << MESSAGE_ID_SERVER_SHIFT;
    assert( (server_shifted & ~MESSAGE_ID_SERVER_BITS) == 0 );
    return (server_shifted & MESSAGE_ID_SERVER_BITS) | (id_src & ~MESSAGE_ID_SERVER_BITS);
}

OriginID GetUniqueIDOriginID(uint32 uid) {
    uint32 server_int = ( uid & MESSAGE_ID_SERVER_BITS ) >> MESSAGE_ID_SERVER_SHIFT;
    OriginID id;
    id.id = server_int;
    return id;
}

uint32 GetUniqueIDMessageID(uint32 uid) {
    return ( uid & ~MESSAGE_ID_SERVER_BITS );
}


uint32 Message::sIDSource = 0;

Message::Message(const OriginID& origin, bool x)
 : mID( GenerateUniqueID(origin, sIDSource++) )
{
}

Message::Message(uint32 _id)
 : mID(_id)
{
}

Message::~Message() {
}

uint32 Message::id() const {
    return mID;
}

uint32 Message::deserialize(const Network::Chunk& wire, uint32 offset, Message** result) {
    uint8 raw_type;
    uint32 _id;

    memcpy( &raw_type, &wire[offset], 1 );
    offset += 1;

    memcpy( &_id, &wire[offset], sizeof(uint32) );
    offset += sizeof(uint32);

    Message* msg = NULL;

    switch(raw_type) {
      case MESSAGE_TYPE_PROXIMITY:
        msg = new ProximityMessage(wire, offset, _id);
        break;
      case MESSAGE_TYPE_LOCATION:
        msg = new LocationMessage(wire, offset, _id);
        break;
      case MESSAGE_TYPE_SUBSCRIPTION:
        msg = new SubscriptionMessage(wire, offset, _id);
        break;
      case MESSAGE_TYPE_MIGRATE:
        msg = new MigrateMessage(wire, offset, _id);
        break;
      case MESSAGE_TYPE_CSEG_CHANGE:
        msg = new CSegChangeMessage(wire, offset, _id);
        break;
      case MESSAGE_TYPE_LOAD_STATUS:
        msg = new LoadStatusMessage(wire, offset, _id);
        break;
      default:
        assert(false);
        break;
    }

    if (result != NULL)
        *result = msg;
    else // if they didn't want the result, just delete it
        delete msg;

    return offset;
}

uint32 Message::serializeHeader(Network::Chunk& wire, uint32 offset) {
    if (wire.size() < offset + sizeof(MessageType) + sizeof(uint32) )
        wire.resize( offset + sizeof(MessageType) + sizeof(uint32) );

    MessageType t = type();
    memcpy( &wire[offset], &t, sizeof(MessageType) );
    offset += sizeof(MessageType);

    memcpy( &wire[offset], &mID, sizeof(uint32) );
    offset += sizeof(uint32);

    return offset;
}



ProximityMessage::ProximityMessage(const OriginID& origin, const UUID& dst_object, const UUID& nbr, EventType evt, const TimedMotionVector3f& loc)
 : Message(origin, true),
   mDestObject(dst_object),
   mNeighbor(nbr),
   mEvent(evt),
   mLocation(loc)
{
}

ProximityMessage::ProximityMessage(const Network::Chunk& wire, uint32& offset, uint32 _id)
 : Message(_id)
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

    Time t(0);
    Vector3f pos;
    Vector3f vel;

    memcpy( &t, &wire[offset], sizeof(Time) );
    offset += sizeof(Time);

    memcpy( &pos, &wire[offset], sizeof(Vector3f) );
    offset += sizeof(Vector3f);

    memcpy( &vel, &wire[offset], sizeof(Vector3f) );
    offset += sizeof(Vector3f);

    mLocation = TimedMotionVector3f( t, MotionVector3f(pos, vel) );
}

MessageType ProximityMessage::type() const {
    return MESSAGE_TYPE_PROXIMITY;
}

const UUID& ProximityMessage::destObject() const {
    return mDestObject;
}

const UUID& ProximityMessage::neighbor() const {
    return mNeighbor;
}

const TimedMotionVector3f& ProximityMessage::location() const {
    return mLocation;
}

const ProximityMessage::EventType ProximityMessage::event() const {
    return mEvent;
}

uint32 ProximityMessage::serialize(Network::Chunk& wire, uint32 offset) {
    offset = serializeHeader(wire, offset);

    uint32 prox_part_size = 2 * UUID::static_size + 1;
    wire.resize( wire.size() + prox_part_size );
    uint32 loc_part_size = sizeof(Time) + 2 * sizeof(Vector3f);
    wire.resize( wire.size() + loc_part_size );

    memcpy( &wire[offset], mDestObject.getArray().data(), UUID::static_size );
    offset += UUID::static_size;

    memcpy( &wire[offset], mNeighbor.getArray().data(), UUID::static_size );
    offset += UUID::static_size;

    uint8 evt_type = (uint8)mEvent;
    memcpy( &wire[offset], &evt_type, 1 );
    offset += 1;

    Time t = mLocation.time();
    memcpy( &wire[offset], &t, sizeof(Time) );
    offset += sizeof(Time);

    memcpy( &wire[offset], &mLocation.value().position(), sizeof(Vector3f) );
    offset += sizeof(Vector3f);

    memcpy( &wire[offset], &mLocation.value().velocity(), sizeof(Vector3f) );
    offset += sizeof(Vector3f);

    return offset;
}



ObjectToObjectMessage::ObjectToObjectMessage(const OriginID& origin, const UUID& src_object, const UUID& dest_object)
 : Message(origin, true),
   mSourceObject(src_object),
   mDestObject(dest_object)
{
}

ObjectToObjectMessage::ObjectToObjectMessage(const Network::Chunk& wire, uint32& offset, uint32 _id)
 : Message(_id)
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




LocationMessage::LocationMessage(const OriginID& origin, const UUID& src_object, const UUID& dest_object, const TimedMotionVector3f& loc)
 : ObjectToObjectMessage(origin, src_object, dest_object),
   mLocation(loc)
{
}

LocationMessage::LocationMessage(const Network::Chunk& wire, uint32& offset, uint32 _id)
 : ObjectToObjectMessage(wire, offset, _id),
   mLocation( Time(0), MotionVector3f() )
{
    Time t(0);
    Vector3f pos;
    Vector3f vel;

    memcpy( &t, &wire[offset], sizeof(Time) );
    offset += sizeof(Time);

    memcpy( &pos, &wire[offset], sizeof(Vector3f) );
    offset += sizeof(Vector3f);

    memcpy( &vel, &wire[offset], sizeof(Vector3f) );
    offset += sizeof(Vector3f);

    mLocation = TimedMotionVector3f( t, MotionVector3f(pos, vel) );
}

MessageType LocationMessage::type() const {    
    return MESSAGE_TYPE_LOCATION;
}

const TimedMotionVector3f& LocationMessage::location() const {
    return mLocation;
}

uint32 LocationMessage::serialize(Network::Chunk& wire, uint32 offset) {
    offset = serializeHeader(wire, offset);
    offset = serializeSourceDest(wire, offset);

    uint32 loc_part_size = sizeof(Time) + 2 * sizeof(Vector3f);
    wire.resize( wire.size() + loc_part_size );

    Time t = mLocation.time();
    memcpy( &wire[offset], &t, sizeof(Time) );
    offset += sizeof(Time);

    memcpy( &wire[offset], &mLocation.value().position(), sizeof(Vector3f) );
    offset += sizeof(Vector3f);

    memcpy( &wire[offset], &mLocation.value().velocity(), sizeof(Vector3f) );
    offset += sizeof(Vector3f);

    return offset;
}



SubscriptionMessage::SubscriptionMessage(const OriginID& origin, const UUID& src_object, const UUID& dest_object, const Action& act)
 : ObjectToObjectMessage(origin, src_object, dest_object),
   mAction(act)
{
}

SubscriptionMessage::SubscriptionMessage(const Network::Chunk& wire, uint32& offset, uint32 _id)
 : ObjectToObjectMessage(wire, offset, _id)
{
    uint8 raw_act;
    memcpy( &raw_act, &wire[offset], sizeof(uint8) );
    mAction = (Action)raw_act;
    offset += sizeof(uint8);
}

MessageType SubscriptionMessage::type() const {
    return MESSAGE_TYPE_SUBSCRIPTION;
}

const SubscriptionMessage::Action& SubscriptionMessage::action() const {
    return mAction;
}

uint32 SubscriptionMessage::serialize(Network::Chunk& wire, uint32 offset) {
    offset = serializeHeader(wire, offset);
    offset = serializeSourceDest(wire, offset);

    uint32 sub_part_size = sizeof(uint8);
    wire.resize( wire.size() + sub_part_size );

    uint8 act = (uint8)mAction;
    memcpy( &wire[offset], &act, sizeof(uint8) );
    offset += sizeof(uint8);

    return offset;
}




MigrateMessage::MigrateMessage(const OriginID& origin, const UUID& obj, float proxRadius, uint16_t subscriberCount)
 : Message(origin, true),
   mObject(obj),
   mProximityRadius(proxRadius),
   mCountSubscribers(subscriberCount)
{

  mSubscribers = new UUID[mCountSubscribers];

}

MigrateMessage::MigrateMessage(const Network::Chunk& wire, uint32& offset, uint32 _id)
 : Message(_id)
{
    uint8 raw_object[UUID::static_size];
    memcpy( &raw_object, &wire[offset], UUID::static_size );
    offset += UUID::static_size;
    mObject = UUID(raw_object, UUID::static_size);

    float proximityRadius;
    memcpy(&proximityRadius, &wire[offset], sizeof(proximityRadius));
    offset += sizeof(float);
    mProximityRadius = proximityRadius;

    uint16_t countSubscribers;
    memcpy(&countSubscribers, &wire[offset], sizeof(countSubscribers));
    offset += sizeof(uint16_t);
    mCountSubscribers = countSubscribers;

    mSubscribers = new UUID[mCountSubscribers];

    for (int i=0; i < mCountSubscribers; i++) {
      memcpy( &raw_object, &wire[offset], UUID::static_size );
      offset += UUID::static_size;
      mSubscribers[i] = UUID(raw_object, UUID::static_size);
    }
}

MigrateMessage::~MigrateMessage() {
    if (mCountSubscribers > 0)
        delete mSubscribers;
}

MessageType MigrateMessage::type() const {
    return MESSAGE_TYPE_MIGRATE;
}

const UUID& MigrateMessage::object() const {
    return mObject;
}

uint32 MigrateMessage::serialize(Network::Chunk& wire, uint32 offset) {
    offset = serializeHeader(wire, offset);

    uint32 uuid_size = UUID::static_size;
    wire.resize( wire.size() + uuid_size + sizeof(mProximityRadius) +
		 sizeof(mCountSubscribers) + uuid_size*mCountSubscribers);

    memcpy( &wire[offset], mObject.getArray().data(), uuid_size );
    offset += uuid_size;

    memcpy( &wire[offset], &mProximityRadius, sizeof(mProximityRadius) );
    offset += sizeof(mProximityRadius);

    memcpy( &wire[offset], &mCountSubscribers, sizeof(mCountSubscribers) );
    offset += sizeof(mCountSubscribers);

    for (int i=0; i < mCountSubscribers; i++) {
      memcpy( &wire[offset], mSubscribers[i].getArray().data(), uuid_size);
      offset += uuid_size;
    }

    return offset;
}

const float MigrateMessage::proximityRadius() const {
    return mProximityRadius;
}

const int MigrateMessage::subscriberCount() const {
    return mCountSubscribers;
}

UUID* MigrateMessage::subscriberList() const {
  return mSubscribers;
}

CSegChangeMessage::CSegChangeMessage(const OriginID& origin, uint8_t number_of_regions)
 : Message(origin, true),
   mNumberOfRegions(number_of_regions)
{
  mSplitRegions = new SplitRegion<float>[number_of_regions];
}

CSegChangeMessage::CSegChangeMessage(const Network::Chunk& wire, uint32& offset, uint32 _id)
 : Message(_id)
{
    uint8_t number_of_regions;
    memcpy(&number_of_regions, &wire[offset], sizeof(number_of_regions));
    offset += sizeof(number_of_regions);
    mNumberOfRegions = number_of_regions;

    mSplitRegions = new SplitRegionf[number_of_regions];
    
    for (int i=0; i < mNumberOfRegions; i++) {
      memcpy( &mSplitRegions[i], &wire[offset], sizeof(SplitRegion<float>) );
      offset += sizeof(SplitRegionf);      
    }
}

CSegChangeMessage::~CSegChangeMessage() {
    delete mSplitRegions;
}

MessageType CSegChangeMessage::type() const {
  return MESSAGE_TYPE_CSEG_CHANGE;
}

uint32 CSegChangeMessage::serialize(Network::Chunk& wire, uint32 offset) {
    offset = serializeHeader(wire, offset);
    
    wire.resize( wire.size() +  sizeof(mNumberOfRegions) 
		 + mNumberOfRegions*sizeof(SplitRegionf) );
    
    memcpy( &wire[offset], &mNumberOfRegions, sizeof(mNumberOfRegions) );
    offset += sizeof(mNumberOfRegions);

    for (int i=0; i < mNumberOfRegions; i++) {
      memcpy(&wire[offset], &mSplitRegions[i], sizeof(SplitRegionf));
      offset += sizeof(SplitRegionf);
    }

    return offset;
}

const uint8_t CSegChangeMessage::numberOfRegions() const {
    return mNumberOfRegions;
}

SplitRegionf* CSegChangeMessage::splitRegions() const {
    return mSplitRegions;
}

LoadStatusMessage::LoadStatusMessage(const OriginID& origin, float load_reading)
 : Message(origin, true),
   mLoadReading(load_reading)
{
  
}

LoadStatusMessage::LoadStatusMessage(const Network::Chunk& wire, uint32& offset, uint32 _id)
 : Message(_id)
{
    float load_reading;
    memcpy(&load_reading, &wire[offset], sizeof(load_reading));
    offset += sizeof(load_reading);
    mLoadReading = load_reading;
}

LoadStatusMessage::~LoadStatusMessage() {
}

MessageType LoadStatusMessage::type() const {
  return MESSAGE_TYPE_LOAD_STATUS;
}

uint32 LoadStatusMessage::serialize(Network::Chunk& wire, uint32 offset) {
    offset = serializeHeader(wire, offset);
    
    wire.resize( wire.size() +  sizeof(mLoadReading) );
    
    memcpy( &wire[offset], &mLoadReading, sizeof(mLoadReading) );
    offset += sizeof(mLoadReading);

    return offset;
}

const float LoadStatusMessage::loadReading() const {
    return mLoadReading;
}



} // namespace CBR

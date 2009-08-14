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

#define MESSAGE_ID_SERVER_SHIFT 52
#define MESSAGE_ID_SERVER_BITS 0xFFF0000000000000LL


uint64 sIDSource = 0;

uint64 GenerateUniqueID(const ServerID& origin) {
    uint64 id_src = sIDSource++;
    uint64 message_id_server_bits=MESSAGE_ID_SERVER_BITS;
    uint64 server_int = (uint64)origin;
    uint64 server_shifted = server_int << MESSAGE_ID_SERVER_SHIFT;
    assert( (server_shifted & ~message_id_server_bits) == 0 );
    return (server_shifted & message_id_server_bits) | (id_src & ~message_id_server_bits);
}

ServerID GetUniqueIDServerID(uint64 uid) {
    uint64 message_id_server_bits=MESSAGE_ID_SERVER_BITS;
    uint64 server_int = ( uid & message_id_server_bits ) >> MESSAGE_ID_SERVER_SHIFT;
    return (ServerID) server_int;
}

uint64 GetUniqueIDMessageID(uint64 uid) {
    uint64 message_id_server_bits=MESSAGE_ID_SERVER_BITS;
    return ( uid & ~message_id_server_bits );
}


Message::Message(const ServerID& origin, bool x)
 : mID( GenerateUniqueID(origin) )
{
}

Message::Message(uint64 _id)
 : mID(_id)
{
}

Message::~Message() {
}

uint64 Message::id() const {
    return mID;
}

uint32 Message::deserialize(const Network::Chunk& wire, uint32 offset, Message** result) {
    uint8 raw_type;
    uint64 _id;

    memcpy( &raw_type, &wire[offset], 1 );
    offset += 1;

    memcpy( &_id, &wire[offset], sizeof(uint64) );
    offset += sizeof(uint64);

    Message* msg = NULL;

    switch(raw_type) {
      case MESSAGE_TYPE_OBJECT:
        msg = new ObjectMessage(wire, offset, _id);
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
      case MESSAGE_TYPE_OSEG_MIGRATE:
        msg = new OSegMigrateMessage(wire,offset,_id);
        break;
      case MESSAGE_TYPE_NOISE:
        msg = new NoiseMessage(wire,offset,_id);
        break;
      case MESSAGE_TYPE_SERVER_PROX_QUERY:
        msg = new ServerProximityQueryMessage(wire,offset,_id);
        break;
      case MESSAGE_TYPE_SERVER_PROX_RESULT:
        msg = new ServerProximityResultMessage(wire,offset,_id);
        break;
      case MESSAGE_TYPE_BULK_LOCATION:
        msg = new BulkLocationMessage(wire,offset,_id);
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
    if (wire.size() < offset + sizeof(MessageType) + sizeof(uint64) )
        wire.resize( offset + sizeof(MessageType) + sizeof(uint64) );

    MessageType t = type();
    memcpy( &wire[offset], &t, sizeof(MessageType) );
    offset += sizeof(MessageType);

    memcpy( &wire[offset], &mID, sizeof(uint64) );
    offset += sizeof(uint64);

    return offset;
}

void MessageDispatcher::registerMessageRecipient(MessageType type, MessageRecipient* recipient) {
    MessageRecipientMap::iterator it = mMessageRecipients.find(type);
    if (it != mMessageRecipients.end()) {
        // FIXME warn that we are overriding old recipient
    }

    mMessageRecipients[type] = recipient;
}

void MessageDispatcher::unregisterMessageRecipient(MessageType type, MessageRecipient* recipient) {
    MessageRecipientMap::iterator it = mMessageRecipients.find(type);
    if (it == mMessageRecipients.end()) {
        // FIXME warn that we are trying to remove an recipient that hasn't been registered
        return;
    }

    if (it->second != recipient) {
        // FIXME warn that we tried to remove the wrong recipient
        return;
    }

    mMessageRecipients.erase(it);
}

void MessageDispatcher::dispatchMessage(Message* msg) const {
    if (msg == NULL) return;

    MessageRecipientMap::const_iterator it = mMessageRecipients.find(msg->type());

    if (it == mMessageRecipients.end()) {
        // FIXME log warning
        return;
    }

    MessageRecipient* recipient = it->second;
    recipient->receiveMessage(msg);
}


// Specific message types

template<typename PBJMessageType>
uint32 serializePBJMessage(const PBJMessageType& contents, Network::Chunk& wire, uint32 offset) {
    std::string payload;
    bool serialized_success = contents.SerializeToString(&payload);
    assert(serialized_success);
    wire.resize( wire.size() + payload.size() );
    memcpy(&wire[offset], &payload[0], payload.size());
    offset += payload.size();

    return offset;
}

template<typename PBJMessageType>
void parsePBJMessage(PBJMessageType& contents, const Network::Chunk& wire, uint32& offset) {
    bool parse_success = contents.ParseFromArray((void*)&wire[offset], wire.size() - offset);
    assert(parse_success);
    offset = wire.size();
}



ObjectMessage::ObjectMessage(const ServerID& origin,
    const UUID& src, const uint32 src_port,
    const UUID& dest, const uint32 dest_port,
    const Network::Chunk& payload)
 : Message(origin, true)
{
    contents.set_source_object(src);
    contents.set_source_port(src_port);
    contents.set_dest_object(dest);
    contents.set_dest_port(dest_port);
    contents.set_payload(&payload[0], payload.size());
}

ObjectMessage::ObjectMessage(const ServerID& origin, const CBR::Protocol::Object::ObjectMessage& src)
 : Message(origin, true),
   contents(src)
{
}

ObjectMessage::ObjectMessage(const Network::Chunk& wire, uint32& offset, uint64 _id)
 : Message(_id)
{
    parsePBJMessage(contents, wire, offset);
}

MessageType ObjectMessage::type() const {
    return MESSAGE_TYPE_OBJECT;
}

uint32 ObjectMessage::serialize(Network::Chunk& wire, uint32 offset) {
    offset = serializeHeader(wire, offset);
    return serializePBJMessage(contents, wire, offset);
}





NoiseMessage::NoiseMessage(const ServerID& origin, uint32 noise_sz)
 : Message(origin, true),
   mNoiseSize(noise_sz)
{
}

NoiseMessage::NoiseMessage(const Network::Chunk& wire, uint32& offset, uint64 _id)
 : Message(_id)
{
    memcpy( &mNoiseSize, &wire[offset], sizeof(uint32) );
    offset += sizeof(uint32);

    // Advance offset without reading any data, by mNoiseSize bytes
    offset += mNoiseSize;
}

MessageType NoiseMessage::type() const {
    return MESSAGE_TYPE_NOISE;
}

uint32 NoiseMessage::serialize(Network::Chunk& wire, uint32 offset) {
    offset = serializeHeader(wire, offset);

    wire.resize( wire.size() + sizeof(uint32) );
    memcpy( &wire[offset], &mNoiseSize, sizeof(uint32) );
    offset += sizeof(uint32);

    // Just expand by the number of bytes we want, don't bother setting them to anything
    wire.resize( wire.size() + mNoiseSize );
    offset += mNoiseSize;

    return offset;
}



MigrateMessage::MigrateMessage(const ServerID& origin)
 : Message(origin, true)
{
}

MigrateMessage::MigrateMessage(const Network::Chunk& wire, uint32& offset, uint64 _id)
 : Message(_id)
{
    parsePBJMessage(contents, wire, offset);
}

MessageType MigrateMessage::type() const {
    return MESSAGE_TYPE_MIGRATE;
}

uint32 MigrateMessage::serialize(Network::Chunk& wire, uint32 offset) {
    offset = serializeHeader(wire, offset);
    return serializePBJMessage(contents, wire, offset);
}



CSegChangeMessage::CSegChangeMessage(const ServerID& origin, uint8_t number_of_regions)
 : Message(origin, true),
   mNumberOfRegions(number_of_regions)
{
  mSplitRegions = new SplitRegion<float>[number_of_regions];
}

CSegChangeMessage::CSegChangeMessage(const Network::Chunk& wire, uint32& offset, uint64 _id)
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

LoadStatusMessage::LoadStatusMessage(const ServerID& origin, float load_reading)
 : Message(origin, true),
   mLoadReading(load_reading)
{

}

LoadStatusMessage::LoadStatusMessage(const Network::Chunk& wire, uint32& offset, uint64 _id)
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



//*****************OSEG messages

////migrate message
//constructor
OSegMigrateMessage::OSegMigrateMessage(const ServerID& origin,ServerID sID_from, ServerID sID_to, ServerID sMessageDest, ServerID sMessageFrom, UUID obj_id, OSegMigrateMessage::OSegMigrateAction action)
 : Message(origin, true),
   mServID_from (sID_from),
   mServID_to   (sID_to),
   mMessageDestination (sMessageDest),
   mMessageFrom (sMessageFrom),
   mObjID       (obj_id),
   mAction      (action)
{

}


//constructor
OSegMigrateMessage::OSegMigrateMessage(const Network::Chunk& wire, uint32& offset, uint64 _id)
 : Message(_id)
{

  ServerID     sID_from, sID_to, sMessageDest, sMessageFrom;
  UUID                                               obj_id;
  OSegMigrateAction                                  action;

  //copying mServID_from
  memcpy(&sID_from, &wire[offset], sizeof(ServerID));
  offset += sizeof(ServerID);
  mServID_from = sID_from;

  //copying mServID_to
  memcpy(&sID_to, &wire[offset], sizeof(ServerID));
  offset += sizeof(ServerID);
  mServID_to = sID_to;

  //copying mMessageDestination
  memcpy(&sMessageDest,&wire[offset],sizeof(ServerID));
  offset += sizeof(ServerID);
  mMessageDestination = sMessageDest;

  //copying mMessageFrom
  memcpy(&sMessageFrom,&wire[offset],sizeof(ServerID));
  offset += sizeof(ServerID);
  mMessageFrom = sMessageFrom;

  //copying mObjID
  memcpy(&obj_id, &wire[offset], sizeof(UUID));
  offset += sizeof(UUID);
  mObjID = obj_id;

  //copying mAction
  memcpy(&action, &wire[offset], sizeof(OSegMigrateAction));
  offset += sizeof(OSegMigrateAction);
  mAction = action;

}

//destructor
OSegMigrateMessage::~OSegMigrateMessage()
{
  //need to complete later
}

MessageType OSegMigrateMessage::type() const
{
  return MESSAGE_TYPE_OSEG_MIGRATE;
}

/*
  Serializer.  Should work.
*/
uint32 OSegMigrateMessage::serialize(Network::Chunk& wire, uint32 offset)
{

  offset = serializeHeader(wire,offset);

  wire.resize(wire.size() + sizeof(ServerID) + sizeof(ServerID) + sizeof(ServerID) + sizeof(ServerID) + sizeof(UUID) + sizeof(OSegMigrateAction));
  //          wire            sID_from           sID_to              mMessageDest       mMessageFrom      obj_id           action

  memcpy(&wire[offset], &mServID_from, sizeof(ServerID)); //copying sID_from
  offset += sizeof(ServerID);

  memcpy(&wire[offset], &mServID_to, sizeof(ServerID)); //copying sID_to
  offset += sizeof(ServerID);

  memcpy(&wire[offset], &mMessageDestination, sizeof(ServerID)); //copying mMessageDestination
  offset += sizeof(ServerID);

  memcpy(&wire[offset], &mMessageFrom, sizeof(ServerID)); //copying mMessageFrom
  offset += sizeof(ServerID);

  memcpy(&wire[offset], &mObjID, sizeof(UUID));  //copying obj_id
  offset += sizeof(UUID);

  memcpy(&wire[offset], &mAction, sizeof(OSegMigrateAction)); //copying action
  offset += sizeof(OSegMigrateAction);

  return offset;
}

//Sequence of osegmessage accessors
ServerID OSegMigrateMessage::getServFrom()
{
  return mServID_from;
}
ServerID OSegMigrateMessage::getServTo()
{
  return mServID_to;
}
UUID OSegMigrateMessage::getObjID()
{
  return mObjID;
}
OSegMigrateMessage::OSegMigrateAction OSegMigrateMessage::getAction()
{
  return mAction;
}
ServerID  OSegMigrateMessage::getMessageDestination()
{
  return mMessageDestination;
}

ServerID OSegMigrateMessage::getMessageFrom()
{
  return mMessageFrom;
}


/////OSEG lookup

//primary constructor
OSegLookupMessage::OSegLookupMessage(const ServerID& origin,ServerID sID_seeker, ServerID sID_keeper, UUID obj_id, OSegLookupAction action)
   : Message(origin, true),
     mSeeker(sID_seeker),
     mKeeper(sID_keeper),
     mObjID(obj_id),
     mAction(action)
{
  //nothing to do here.
}

//secondary constructor
OSegLookupMessage::OSegLookupMessage(const Network::Chunk& wire, uint32& offset, uint64 _id)
  : Message(_id)
{
  ServerID sID_seeker,sID_keeper;
  UUID obj_id;
  OSegLookupAction action;

  //seeker
  memcpy(&sID_seeker,&wire[offset],sizeof(ServerID));
  offset += sizeof(ServerID);
  mSeeker = sID_seeker;

  //keeper
  memcpy(&sID_keeper,&wire[offset],sizeof(ServerID));
  offset += sizeof(ServerID);
  mKeeper = sID_keeper;

  //obj id
  memcpy(&obj_id,&wire[offset],sizeof(UUID));
  offset += sizeof(UUID);
  mObjID = obj_id;

  //action
  memcpy(&action,&wire[offset],sizeof(OSegLookupAction));
  offset += sizeof(OSegLookupAction);
  mAction = action;

}


uint32 OSegLookupMessage::serialize(Network::Chunk& wire, uint32 offset)
{
  offset = serializeHeader(wire,offset);

  wire.resize(wire.size() + sizeof(ServerID) + sizeof(ServerID) + sizeof(UUID) + sizeof(OSegLookupAction));
  //          wire            mSeeker            mKeeper            mObjID            mAction

  memcpy(&wire[offset], &mSeeker, sizeof(ServerID)); //copying seeker
  offset += sizeof(ServerID);

  memcpy(&wire[offset], &mKeeper, sizeof(ServerID)); //copying keeper
  offset += sizeof(ServerID);

  memcpy(&wire[offset], &mObjID, sizeof(UUID)); //copying objid
  offset += sizeof(UUID);

  memcpy(&wire[offset], &mAction, sizeof(OSegLookupAction)); //copying action
  offset += sizeof(UUID);

  return offset;
}

//nothing to do in the destructor.
OSegLookupMessage::~OSegLookupMessage()
{

}

//some basic accessors.
ServerID OSegLookupMessage::getSeeker()
{
  return mSeeker;
}
ServerID OSegLookupMessage::getKeeper()
{
  return mKeeper;
}
UUID OSegLookupMessage::getObjID()
{
  return mObjID;
}

OSegLookupMessage::OSegLookupAction OSegLookupMessage::getAction()
{
  return mAction;
}

MessageType OSegLookupMessage::type() const
{
  return MESSAGE_TYPE_OSEG_LOOKUP;
}





ServerProximityQueryMessage::ServerProximityQueryMessage(const ServerID& origin)
 : Message(origin, true)
{
}

ServerProximityQueryMessage::ServerProximityQueryMessage(const Network::Chunk& wire, uint32& offset, uint64 _id)
 : Message(_id)
{
    parsePBJMessage(contents, wire, offset);
}

ServerProximityQueryMessage::~ServerProximityQueryMessage() {
}

MessageType ServerProximityQueryMessage::type() const {
  return MESSAGE_TYPE_SERVER_PROX_QUERY;
}

uint32 ServerProximityQueryMessage::serialize(Network::Chunk& wire, uint32 offset) {
    offset = serializeHeader(wire, offset);
    return serializePBJMessage(contents, wire, offset);
}




ServerProximityResultMessage::ServerProximityResultMessage(const ServerID& origin)
 : Message(origin, true)
{
}

ServerProximityResultMessage::ServerProximityResultMessage(const Network::Chunk& wire, uint32& offset, uint64 _id)
 : Message(_id)
{
    parsePBJMessage(contents, wire, offset);
}

ServerProximityResultMessage::~ServerProximityResultMessage() {
}

MessageType ServerProximityResultMessage::type() const {
  return MESSAGE_TYPE_SERVER_PROX_RESULT;
}

uint32 ServerProximityResultMessage::serialize(Network::Chunk& wire, uint32 offset) {
    offset = serializeHeader(wire, offset);
    return serializePBJMessage(contents, wire, offset);
}




BulkLocationMessage::BulkLocationMessage(const ServerID& origin)
 : Message(origin, true)
{
}

BulkLocationMessage::BulkLocationMessage(const Network::Chunk& wire, uint32& offset, uint64 _id)
 : Message(_id)
{
    parsePBJMessage(contents, wire, offset);
}

BulkLocationMessage::~BulkLocationMessage() {
}

MessageType BulkLocationMessage::type() const {
  return MESSAGE_TYPE_BULK_LOCATION;
}

uint32 BulkLocationMessage::serialize(Network::Chunk& wire, uint32 offset) {
    offset = serializeHeader(wire, offset);
    return serializePBJMessage(contents, wire, offset);
}


} // namespace CBR

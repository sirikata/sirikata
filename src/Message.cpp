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
      case MESSAGE_TYPE_OSEG_MIGRATE_MOVE:
        msg = new OSegMigrateMessageMove(wire,offset,_id);
        break;
      case MESSAGE_TYPE_KILL_OBJ_CONN:
        msg = new KillObjConnMessage(wire,offset,_id);
        break;
      case MESSAGE_TYPE_OSEG_MIGRATE_ACKNOWLEDGE:
        msg = new OSegMigrateMessageAcknowledge(wire,offset,_id);
        break;
      case MESSAGE_TYPE_UPDATE_OSEG:
        msg = new UpdateOSegMessage(wire,offset,_id);
        break;
      case MESSAGE_TYPE_OSEG_ADDED_OBJECT:
        msg = new  OSegAddMessage(wire,offset,_id);
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

uint32 Message::headerSize()const {
    return sizeof(MessageType) + sizeof(uint64);
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

void MessageDispatcher::registerObjectMessageRecipient(uint16 port, ObjectMessageRecipient* recipient) {
    ObjectMessageRecipientMap::iterator it = mObjectMessageRecipients.find(port);
    if (it != mObjectMessageRecipients.end()) {
        // FIXME warn that we are overriding old recipient
    }

    mObjectMessageRecipients[port] = recipient;
}

void MessageDispatcher::unregisterObjectMessageRecipient(uint16 port, ObjectMessageRecipient* recipient) {
    ObjectMessageRecipientMap::iterator it = mObjectMessageRecipients.find(port);
    if (it == mObjectMessageRecipients.end()) {
        // FIXME warn that we are trying to remove an recipient that hasn't been registered
        return;
    }

    if (it->second != recipient) {
        // FIXME warn that we tried to remove the wrong recipient
        return;
    }

    mObjectMessageRecipients.erase(it);
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

void MessageDispatcher::dispatchMessage(const CBR::Protocol::Object::ObjectMessage& msg) const {
    // This is on the space server, so we should only be calling this if the dest is the space
    assert(msg.dest_object() == UUID::null());
    ObjectMessageRecipientMap::const_iterator it = mObjectMessageRecipients.find(msg.dest_port());

    if (it == mObjectMessageRecipients.end()) {
        // FIXME log warning
        return;
    }

    ObjectMessageRecipient* recipient = it->second;
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


size_t ObjectMessage::size()const {
    return contents.ByteSize()+headerSize();
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



CSegChangeMessage::CSegChangeMessage(const ServerID& origin)
 : Message(origin, true)
{
}

CSegChangeMessage::CSegChangeMessage(const Network::Chunk& wire, uint32& offset, uint64 _id)
 : Message(_id)
{
    parsePBJMessage(contents, wire, offset);
}

MessageType CSegChangeMessage::type() const {
  return MESSAGE_TYPE_CSEG_CHANGE;
}

uint32 CSegChangeMessage::serialize(Network::Chunk& wire, uint32 offset) {
    offset = serializeHeader(wire, offset);
    return serializePBJMessage(contents, wire, offset);
}



LoadStatusMessage::LoadStatusMessage(const ServerID& origin)
 : Message(origin, true)
{
}

LoadStatusMessage::LoadStatusMessage(const Network::Chunk& wire, uint32& offset, uint64 _id)
 : Message(_id)
{
    parsePBJMessage(contents, wire, offset);
}

MessageType LoadStatusMessage::type() const {
  return MESSAGE_TYPE_LOAD_STATUS;
}

uint32 LoadStatusMessage::serialize(Network::Chunk& wire, uint32 offset) {
    offset = serializeHeader(wire, offset);
    return serializePBJMessage(contents, wire, offset);
}



//*****************OSEG messages


OSegMigrateMessageMove::OSegMigrateMessageMove(const ServerID& origin,ServerID sID_from, ServerID sID_to, ServerID sMessageDest, ServerID sMessageFrom, UUID obj_id)
  : Message(origin, true)
{
  contents.set_m_servid_from               (sID_from);
  contents.set_m_servid_to                   (sID_to);
  contents.set_m_message_destination   (sMessageDest);
  contents.set_m_message_from          (sMessageFrom);
  contents.set_m_objid                       (obj_id);
}

OSegMigrateMessageMove::OSegMigrateMessageMove(const Network::Chunk& wire, uint32& offset, uint64 _id)
  : Message(_id)
{
  parsePBJMessage(contents,wire,offset);
}


MessageType OSegMigrateMessageMove::type() const
{
  return MESSAGE_TYPE_OSEG_MIGRATE_MOVE;
}


uint32 OSegMigrateMessageMove::serialize(Network::Chunk& wire, uint32 offset) {
    offset = serializeHeader(wire, offset);
    return serializePBJMessage(contents, wire, offset);
}

ServerID OSegMigrateMessageMove::getServFrom()
{
  return contents.m_servid_from();
}
ServerID OSegMigrateMessageMove::getServTo()
{
  return contents.m_servid_to();
}
UUID OSegMigrateMessageMove::getObjID()
{
  return contents.m_objid();
}

ServerID  OSegMigrateMessageMove::getMessageDestination()
{
  return contents.m_message_destination();
}

ServerID OSegMigrateMessageMove::getMessageFrom()
{
  return contents.m_message_from();
}


//and now ack message
OSegMigrateMessageAcknowledge::OSegMigrateMessageAcknowledge(const ServerID& origin, const ServerID &sID_from, const ServerID &sID_to, const ServerID &sMessageDest, const ServerID &sMessageFrom, const UUID &obj_id)
  : Message(origin, true)
{

  contents.set_m_servid_from               (sID_from);
  contents.set_m_servid_to                   (sID_to);
  contents.set_m_message_destination   (sMessageDest);
  contents.set_m_message_from          (sMessageFrom);
  contents.set_m_objid                       (obj_id);

}

OSegMigrateMessageAcknowledge::OSegMigrateMessageAcknowledge(const Network::Chunk& wire, uint32& offset, uint64 _id)
  : Message(_id)
{
  parsePBJMessage(contents,wire,offset);
}


MessageType OSegMigrateMessageAcknowledge::type() const
{
  return MESSAGE_TYPE_OSEG_MIGRATE_ACKNOWLEDGE;
}


uint32 OSegMigrateMessageAcknowledge::serialize(Network::Chunk& wire, uint32 offset) {
    offset = serializeHeader(wire, offset);
    return serializePBJMessage(contents, wire, offset);
}

ServerID OSegMigrateMessageAcknowledge::getServFrom()
{
  return contents.m_servid_from();
}
ServerID OSegMigrateMessageAcknowledge::getServTo()
{
  return contents.m_servid_to();
}
UUID OSegMigrateMessageAcknowledge::getObjID()
{
  return contents.m_objid();
}

ServerID  OSegMigrateMessageAcknowledge::getMessageDestination()
{
  return contents.m_message_destination();
}

ServerID OSegMigrateMessageAcknowledge::getMessageFrom()
{
  return contents.m_message_from();
}

//end of oseg messages.

//update oseg message:


UpdateOSegMessage::UpdateOSegMessage(const ServerID &sID_sendingMessage, const ServerID &sID_objOn, const UUID &obj_id)
  : Message(sID_sendingMessage, true)
{
  contents.set_servid_sending_update         (sID_sendingMessage);
  contents.set_servid_obj_on                          (sID_objOn);
  contents.set_m_objid                                   (obj_id);
}

MessageType UpdateOSegMessage::type() const
{
  return MESSAGE_TYPE_UPDATE_OSEG;
}

UpdateOSegMessage::~UpdateOSegMessage()
{
}


uint32 UpdateOSegMessage::serialize(Network::Chunk& wire, uint32 offset)
{
  offset = serializeHeader(wire, offset);
  return serializePBJMessage(contents, wire, offset);
}


UpdateOSegMessage::UpdateOSegMessage(const Network::Chunk& wire, uint32& offset, uint64 _id)
  : Message(_id)
{
  parsePBJMessage(contents,wire,offset);
}




//end update oseg message


OSegAddMessage::OSegAddMessage(const ServerID& origin, const UUID& obj_id)
  : Message(origin,true)
{
  contents.set_m_objid(obj_id);
}

OSegAddMessage::~OSegAddMessage()
{

}

MessageType OSegAddMessage::type() const
{
  return MESSAGE_TYPE_OSEG_ADDED_OBJECT;
}

uint32 OSegAddMessage::serialize(Network::Chunk& wire, uint32 offset)
{
  offset = serializeHeader(wire, offset);
  return serializePBJMessage(contents, wire, offset);
}



OSegAddMessage::OSegAddMessage(const Network::Chunk& wire, uint32& offset, uint64 _id)
  : Message(_id)
{
  parsePBJMessage(contents,wire,offset);
}


//obj_conn_kill message:

KillObjConnMessage::KillObjConnMessage(const ServerID& origin)
  : Message(origin,true)
{

}
KillObjConnMessage::~KillObjConnMessage()
{

}

KillObjConnMessage::KillObjConnMessage(const Network::Chunk& wire, uint32& offset, uint64 _id)
  : Message(_id)
{
  parsePBJMessage(contents,wire,offset);
}


MessageType KillObjConnMessage::type() const
{
  return MESSAGE_TYPE_KILL_OBJ_CONN;
}


uint32 KillObjConnMessage::serialize(Network::Chunk& wire, uint32 offset)
{
    offset = serializeHeader(wire, offset);
    return serializePBJMessage(contents, wire, offset);
}

//end of obj_conn_kill message


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


std::string* serializeObjectHostMessage(const CBR::Protocol::Object::ObjectMessage& msg) {
    // FIXME we need a small header for framing purposes
    std::string real_payload = serializePBJMessage(msg);
    char payload_size_buffer[ sizeof(uint32) + sizeof(uint8) ];
    *((uint32*)payload_size_buffer) = real_payload.size(); // FIXME endian
    *(payload_size_buffer + sizeof(uint32)) = 0; // null terminator
    std::string* final_payload = new std::string( std::string(payload_size_buffer,sizeof(uint32)) + real_payload );
    return final_payload;
}

CBR::Protocol::Object::ObjectMessage* createObjectMessage(ServerID source_server, const UUID& src, uint16 src_port, const UUID& dest, uint16 dest_port, const std::string& payload) {
    CBR::Protocol::Object::ObjectMessage* result = new CBR::Protocol::Object::ObjectMessage();

    result->set_source_object(src);
    result->set_source_port(src_port);
    result->set_dest_object(dest);
    result->set_dest_port(dest_port);
    result->set_unique(GenerateUniqueID(source_server));
    result->set_payload(payload);

    return result;
}

} // namespace CBR

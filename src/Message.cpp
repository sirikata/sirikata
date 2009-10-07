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


Message::Message(const ServerID& origin)
 : mID( GenerateUniqueID(origin) ),
   mSource(NullServerID),
   mDest(NullServerID)
{
}

Message::Message(const CBR::Protocol::Server::ServerMessage& svr_msg)
 : mID(svr_msg.id()),
   mSource(svr_msg.source_server()),
   mDest(svr_msg.dest_server())
{
}

Message::~Message() {
}

uint64 Message::id() const {
    return mID;
}

Message* Message::deserialize(const Network::Chunk& wire) {
    CBR::Protocol::Server::ServerMessage svr_msg;
    bool parsed = parsePBJMessage(&svr_msg, wire);

    if (!parsed) {
        SILOG(msg,warning,"[MSG] Couldn't parse message.");
        return NULL;
    }

    Message* msg = NULL;

    switch( svr_msg.dest_port() ) {
      case MESSAGE_TYPE_OBJECT:
        msg = new ObjectMessage(svr_msg);
        break;
      case MESSAGE_TYPE_MIGRATE:
        msg = new MigrateMessage(svr_msg);
        break;
      case MESSAGE_TYPE_CSEG_CHANGE:
        msg = new CSegChangeMessage(svr_msg);
        break;
      case MESSAGE_TYPE_LOAD_STATUS:
        msg = new LoadStatusMessage(svr_msg);
        break;
      case MESSAGE_TYPE_OSEG_MIGRATE_MOVE:
        msg = new OSegMigrateMessageMove(svr_msg);
        break;
      case MESSAGE_TYPE_KILL_OBJ_CONN:
        msg = new KillObjConnMessage(svr_msg);
        break;
      case MESSAGE_TYPE_OSEG_MIGRATE_ACKNOWLEDGE:
        msg = new OSegMigrateMessageAcknowledge(svr_msg);
        break;
      case MESSAGE_TYPE_UPDATE_OSEG:
        msg = new UpdateOSegMessage(svr_msg);
        break;
      case MESSAGE_TYPE_OSEG_ADDED_OBJECT:
        msg = new  OSegAddMessage(svr_msg);
        break;
      case MESSAGE_TYPE_NOISE:
        msg = new NoiseMessage(svr_msg);
        break;
      case MESSAGE_TYPE_SERVER_PROX_QUERY:
        msg = new ServerProximityQueryMessage(svr_msg);
        break;
      case MESSAGE_TYPE_SERVER_PROX_RESULT:
        msg = new ServerProximityResultMessage(svr_msg);
        break;
      case MESSAGE_TYPE_BULK_LOCATION:
        msg = new BulkLocationMessage(svr_msg);
        break;
      default:
        assert(false);
        break;
    }

    return msg;
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


ObjectMessage::ObjectMessage(const ServerID& origin, const CBR::Protocol::Object::ObjectMessage& src)
 : Message(origin),
   contents(src)
{
}

ObjectMessage::ObjectMessage(const CBR::Protocol::Server::ServerMessage& svr_msg)
 : Message(svr_msg)
{
    parsePBJMessage(&contents, svr_msg.payload());
}

MessageType ObjectMessage::type() const {
    return MESSAGE_TYPE_OBJECT;
}

bool ObjectMessage::serialize(Network::Chunk* wire) {
    return serializeContents(wire, contents);
}

uint32 ObjectMessage::serializedSize() const {
    return getSerializedSize(contents);
}


NoiseMessage::NoiseMessage(const ServerID& origin, uint32 noise_sz)
 : Message(origin)
{
    std::string payload(noise_sz, 'x');
    contents.set_payload(payload);
}

NoiseMessage::NoiseMessage(const CBR::Protocol::Server::ServerMessage& svr_msg)
 : Message(svr_msg)
{
    parsePBJMessage(&contents, svr_msg.payload());
}

MessageType NoiseMessage::type() const {
    return MESSAGE_TYPE_NOISE;
}

bool NoiseMessage::serialize(Network::Chunk* wire) {
    return serializeContents(wire, contents);
}

uint32 NoiseMessage::serializedSize() const {
    return getSerializedSize(contents);
}




MigrateMessage::MigrateMessage(const ServerID& origin)
 : Message(origin)
{
}

MigrateMessage::MigrateMessage(const CBR::Protocol::Server::ServerMessage& svr_msg)
 : Message(svr_msg)
{
    parsePBJMessage(&contents, svr_msg.payload());
}

MessageType MigrateMessage::type() const {
    return MESSAGE_TYPE_MIGRATE;
}

bool MigrateMessage::serialize(Network::Chunk* wire) {
    return serializeContents(wire, contents);
}

uint32 MigrateMessage::serializedSize() const {
    return getSerializedSize(contents);
}



CSegChangeMessage::CSegChangeMessage(const ServerID& origin)
 : Message(origin)
{
}

CSegChangeMessage::CSegChangeMessage(const CBR::Protocol::Server::ServerMessage& svr_msg)
 : Message(svr_msg)
{
    parsePBJMessage(&contents, svr_msg.payload());
}

MessageType CSegChangeMessage::type() const {
  return MESSAGE_TYPE_CSEG_CHANGE;
}

bool CSegChangeMessage::serialize(Network::Chunk* wire) {
    return serializeContents(wire, contents);
}

uint32 CSegChangeMessage::serializedSize() const {
    return getSerializedSize(contents);
}


LoadStatusMessage::LoadStatusMessage(const ServerID& origin)
 : Message(origin)
{
}

LoadStatusMessage::LoadStatusMessage(const CBR::Protocol::Server::ServerMessage& svr_msg)
 : Message(svr_msg)
{
    parsePBJMessage(&contents, svr_msg.payload());
}

MessageType LoadStatusMessage::type() const {
  return MESSAGE_TYPE_LOAD_STATUS;
}

bool LoadStatusMessage::serialize(Network::Chunk* wire) {
    return serializeContents(wire, contents);
}

uint32 LoadStatusMessage::serializedSize() const {
    return getSerializedSize(contents);
}




//*****************OSEG messages


OSegMigrateMessageMove::OSegMigrateMessageMove(const ServerID& origin,ServerID sID_from, ServerID sID_to, ServerID sMessageDest, ServerID sMessageFrom, UUID obj_id)
  : Message(origin)
{
  contents.set_m_servid_from               (sID_from);
  contents.set_m_servid_to                   (sID_to);
  contents.set_m_message_destination   (sMessageDest);
  contents.set_m_message_from          (sMessageFrom);
  contents.set_m_objid                       (obj_id);
}

OSegMigrateMessageMove::OSegMigrateMessageMove(const CBR::Protocol::Server::ServerMessage& svr_msg)
  : Message(svr_msg)
{
    parsePBJMessage(&contents, svr_msg.payload());
}


MessageType OSegMigrateMessageMove::type() const
{
  return MESSAGE_TYPE_OSEG_MIGRATE_MOVE;
}


bool OSegMigrateMessageMove::serialize(Network::Chunk* wire) {
    return serializeContents(wire, contents);
}

uint32 OSegMigrateMessageMove::serializedSize() const {
    return getSerializedSize(contents);
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
  : Message(origin)
{

  contents.set_m_servid_from               (sID_from);
  contents.set_m_servid_to                   (sID_to);
  contents.set_m_message_destination   (sMessageDest);
  contents.set_m_message_from          (sMessageFrom);
  contents.set_m_objid                       (obj_id);

}

OSegMigrateMessageAcknowledge::OSegMigrateMessageAcknowledge(const CBR::Protocol::Server::ServerMessage& svr_msg)
  : Message(svr_msg)
{
    parsePBJMessage(&contents, svr_msg.payload());
}


MessageType OSegMigrateMessageAcknowledge::type() const
{
  return MESSAGE_TYPE_OSEG_MIGRATE_ACKNOWLEDGE;
}


bool OSegMigrateMessageAcknowledge::serialize(Network::Chunk* wire) {
    return serializeContents(wire, contents);
}

uint32 OSegMigrateMessageAcknowledge::serializedSize() const {
    return getSerializedSize(contents);
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
  : Message(sID_sendingMessage)
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


bool UpdateOSegMessage::serialize(Network::Chunk* wire) {
    return serializeContents(wire, contents);
}

uint32 UpdateOSegMessage::serializedSize() const {
    return getSerializedSize(contents);
}

UpdateOSegMessage::UpdateOSegMessage(const CBR::Protocol::Server::ServerMessage& svr_msg)
  : Message(svr_msg)
{
    parsePBJMessage(&contents, svr_msg.payload());
}




//end update oseg message


OSegAddMessage::OSegAddMessage(const ServerID& origin, const UUID& obj_id)
  : Message(origin)
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

bool OSegAddMessage::serialize(Network::Chunk* wire) {
    return serializeContents(wire, contents);
}

uint32 OSegAddMessage::serializedSize() const {
    return getSerializedSize(contents);
}


OSegAddMessage::OSegAddMessage(const CBR::Protocol::Server::ServerMessage& svr_msg)
  : Message(svr_msg)
{
    parsePBJMessage(&contents, svr_msg.payload());
}


//obj_conn_kill message:

KillObjConnMessage::KillObjConnMessage(const ServerID& origin)
  : Message(origin)
{

}
KillObjConnMessage::~KillObjConnMessage()
{

}

KillObjConnMessage::KillObjConnMessage(const CBR::Protocol::Server::ServerMessage& svr_msg)
  : Message(svr_msg)
{
    parsePBJMessage(&contents, svr_msg.payload());
}


MessageType KillObjConnMessage::type() const
{
  return MESSAGE_TYPE_KILL_OBJ_CONN;
}


bool KillObjConnMessage::serialize(Network::Chunk* wire) {
    return serializeContents(wire, contents);
}

uint32 KillObjConnMessage::serializedSize() const {
    return getSerializedSize(contents);
}

//end of obj_conn_kill message


ServerProximityQueryMessage::ServerProximityQueryMessage(const ServerID& origin)
 : Message(origin)
{
}

ServerProximityQueryMessage::ServerProximityQueryMessage(const CBR::Protocol::Server::ServerMessage& svr_msg)
 : Message(svr_msg)
{
    parsePBJMessage(&contents, svr_msg.payload());
}

ServerProximityQueryMessage::~ServerProximityQueryMessage() {
}

MessageType ServerProximityQueryMessage::type() const {
  return MESSAGE_TYPE_SERVER_PROX_QUERY;
}

bool ServerProximityQueryMessage::serialize(Network::Chunk* wire) {
    return serializeContents(wire, contents);
}

uint32 ServerProximityQueryMessage::serializedSize() const {
    return getSerializedSize(contents);
}



ServerProximityResultMessage::ServerProximityResultMessage(const ServerID& origin)
 : Message(origin)
{
}

ServerProximityResultMessage::ServerProximityResultMessage(const CBR::Protocol::Server::ServerMessage& svr_msg)
 : Message(svr_msg)
{
    parsePBJMessage(&contents, svr_msg.payload());
}

ServerProximityResultMessage::~ServerProximityResultMessage() {
}

MessageType ServerProximityResultMessage::type() const {
  return MESSAGE_TYPE_SERVER_PROX_RESULT;
}

bool ServerProximityResultMessage::serialize(Network::Chunk* wire) {
    return serializeContents(wire, contents);
}

uint32 ServerProximityResultMessage::serializedSize() const {
    return getSerializedSize(contents);
}



BulkLocationMessage::BulkLocationMessage(const ServerID& origin)
 : Message(origin)
{
}

BulkLocationMessage::BulkLocationMessage(const CBR::Protocol::Server::ServerMessage& svr_msg)
 : Message(svr_msg)
{
    parsePBJMessage(&contents, svr_msg.payload());
}

BulkLocationMessage::~BulkLocationMessage() {
}

MessageType BulkLocationMessage::type() const {
  return MESSAGE_TYPE_BULK_LOCATION;
}

bool BulkLocationMessage::serialize(Network::Chunk* wire) {
    return serializeContents(wire, contents);
}

uint32 BulkLocationMessage::serializedSize() const {
    return getSerializedSize(contents);
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

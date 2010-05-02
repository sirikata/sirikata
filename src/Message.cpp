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


void Message::fillMessage(ServerID src, uint16 src_port, ServerID dest, uint16 dest_port) {
    set_source_server(src);
    set_source_port(src_port);
    set_dest_server(dest);
    set_dest_port(dest_port);
}

void Message::fillMessage(ServerID src, uint16 src_port, ServerID dest, ServerID dest_port, const std::string& pl) {
    fillMessage(src, src_port, dest, dest_port);
    set_payload(pl);
}

Message::Message()
 : mCachedSize(0)
{
}

Message::Message(const ServerID& src)
 : mCachedSize(0)
{
    set_source_server(src);
}

Message::Message(ServerID src, uint16 src_port, ServerID dest, ServerID dest_port)
 : mCachedSize(0)
{
    fillMessage(src, src_port, dest, dest_port);
}


Message::Message(ServerID src, uint16 src_port, ServerID dest, uint16 dest_port, const std::string& pl)
 : mCachedSize(0)
{
    fillMessage(src, src_port, dest, dest_port, pl);
}

Message::Message(ServerID src, uint16 src_port, ServerID dest, uint16 dest_port, const CBR::Protocol::Object::ObjectMessage* pl)
 : mCachedSize(0)
{
    fillMessage(src, src_port, dest, dest_port, serializePBJMessage(*pl));
    set_payload_id(pl->unique());
}

void Message::set_source_server(const ServerID sid) {
    mImpl.set_source_server(sid);
    set_id( GenerateUniqueID(sid) );
}

bool Message::serialize(Network::Chunk* output) const {
    // FIXME having to copy here sucks
    std::string result;
    bool success = serializePBJMessage(&result, mImpl);
    if (!success) return false;
    output->resize( result.size() );
    memcpy(&((*output)[0]), &(result[0]), sizeof(uint8)*result.size());
    return true;
}
static char toHex(unsigned char u) {
    if (u<=9) return '0'+u;
    return 'A'+(u-10);
}
static void hexPrint(const char *name, const Network::Chunk&data) {
    std::string str;
    str.resize(data.size()*2);
    for (size_t i=0;i<data.size();++i) {
        str[i*2]=toHex(data[i]%16);
        str[i*2+1]=toHex(data[i]/16);
    }
    std::cout<< name<<' '<<str<<'\n';
}

Message* Message::deserialize(const Network::Chunk& wire) {
    Message* result = new Message();
    bool parsed = result->ParseFromArray( &(wire[0]), wire.size() );
    if (!parsed) {
        hexPrint("Fail",wire);
        SILOG(msg,warning,"[MSG] Couldn't parse message.");
        delete result;
        return NULL;
    }
    result->mCachedSize = wire.size();
    return result;
}

uint32 Message::serializedSize() const {
    if (mCachedSize != 0)
        return mCachedSize;

    return (mCachedSize = mImpl.ByteSize());
}



void ServerMessageDispatcher::registerMessageRecipient(ServerMessagePort type, MessageRecipient* recipient) {
    MessageRecipientMap::iterator it = mMessageRecipients.find(type);
    if (it != mMessageRecipients.end()) {
        // FIXME warn that we are overriding old recipient
    }

    mMessageRecipients[type] = recipient;
}

void ServerMessageDispatcher::unregisterMessageRecipient(ServerMessagePort type, MessageRecipient* recipient) {
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

void ObjectMessageDispatcher::registerObjectMessageRecipient(ObjectMessagePort port, ObjectMessageRecipient* recipient) {
    ObjectMessageRecipientMap::iterator it = mObjectMessageRecipients.find(port);
    if (it != mObjectMessageRecipients.end()) {
        // FIXME warn that we are overriding old recipient
    }

    mObjectMessageRecipients[port] = recipient;
}

void ObjectMessageDispatcher::unregisterObjectMessageRecipient(ObjectMessagePort port, ObjectMessageRecipient* recipient) {
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

void ServerMessageDispatcher::dispatchMessage(Message* msg) const {
    if (msg == NULL) return;

    MessageRecipientMap::const_iterator it = mMessageRecipients.find(msg->dest_port());

    if (it == mMessageRecipients.end()) {
        // FIXME log warning
        return;
    }


    MessageRecipient* recipient = it->second;
    recipient->receiveMessage(msg);
}

void ObjectMessageDispatcher::dispatchMessage(const CBR::Protocol::Object::ObjectMessage& msg) const {
    // This is on the space server, so we should only be calling this if the dest is the space
    //assert(msg.dest_object() == UUID::null());
    ObjectMessageRecipientMap::const_iterator it = mObjectMessageRecipients.find(msg.dest_port());

    if (it == mObjectMessageRecipients.end()) {
        // FIXME log warning
        return;
    }

    ObjectMessageRecipient* recipient = it->second;
    recipient->receiveMessage(msg);
}



void createObjectHostMessage(ObjectHostID source_server, const UUID& src, uint16 src_port, const UUID& dest, uint16 dest_port, const std::string& payload, ObjectMessage* result) {
    if (result == NULL) return;

    result->set_source_object(src);
    result->set_source_port(src_port);
    result->set_dest_object(dest);
    result->set_dest_port(dest_port);
    result->set_unique(GenerateUniqueID(source_server));
    result->set_payload(payload);
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

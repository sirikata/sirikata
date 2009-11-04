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



/* Helper methods for filling in message data in a single line. */
inline void fillMessage(Message* outmsg, ServerID src, uint16 src_port, ServerID dest, uint16 dest_port) {
    outmsg->set_source_server(src);
    outmsg->set_source_port(src_port);
    outmsg->set_dest_server(dest);
    outmsg->set_dest_port(dest_port);
}
inline void fillMessage(Message* outmsg, ServerID src, uint16 src_port, ServerID dest, ServerID dest_port, const std::string& pl) {
    fillMessage(outmsg, src, src_port, dest, dest_port);
    outmsg->set_payload(pl);
}


Message::Message() {
}

Message::Message(const ServerID& src) {
    set_source_server(src);
}

Message::Message(ServerID src, uint16 src_port, ServerID dest, ServerID dest_port) {
    fillMessage(this, src, src_port, dest, dest_port);
}

Message::Message(ServerID src, uint16 src_port, ServerID dest, uint16 dest_port, const std::string& pl) {
    fillMessage(this, src, src_port, dest, dest_port, pl);
}

bool Message::serialize(Network::Chunk* output) {
    // FIXME having to copy here sucks
    std::string result;
    bool success = serializePBJMessage(&result, mImpl);
    if (!success) return false;
    output->resize( result.size() );
    memcpy(&((*output)[0]), &(result[0]), sizeof(uint8)*result.size());
    return true;
}
static char toHex(unsigned char u) {
    if (u>=0&&u<=9) return '0'+u;
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
    return result;
}

uint32 Message::serializedSize() const {
    // FIXME caching would be useful here
    return mImpl.ByteSize();
}



void MessageDispatcher::registerMessageRecipient(ServerMessagePort type, MessageRecipient* recipient) {
    MessageRecipientMap::iterator it = mMessageRecipients.find(type);
    if (it != mMessageRecipients.end()) {
        // FIXME warn that we are overriding old recipient
    }

    mMessageRecipients[type] = recipient;
}

void MessageDispatcher::unregisterMessageRecipient(ServerMessagePort type, MessageRecipient* recipient) {
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

void MessageDispatcher::registerObjectMessageRecipient(ObjectMessagePort port, ObjectMessageRecipient* recipient) {
    ObjectMessageRecipientMap::iterator it = mObjectMessageRecipients.find(port);
    if (it != mObjectMessageRecipients.end()) {
        // FIXME warn that we are overriding old recipient
    }

    mObjectMessageRecipients[port] = recipient;
}

void MessageDispatcher::unregisterObjectMessageRecipient(ObjectMessagePort port, ObjectMessageRecipient* recipient) {
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

    MessageRecipientMap::const_iterator it = mMessageRecipients.find(msg->dest_port());

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



std::string* serializeObjectHostMessage(const CBR::Protocol::Object::ObjectMessage& msg) {
    // FIXME get rid of this, it shouldn't be necessary anymore
    std::string* final_payload = new std::string( serializePBJMessage(msg) );
    return final_payload;
}

ObjectMessage* createObjectHostMessage(ServerID source_server, const UUID& src, uint16 src_port, const UUID& dest, uint16 dest_port, const std::string& payload) {
    ObjectMessage* result = new ObjectMessage();

    result->set_source_object(src);
    result->set_source_port(src_port);
    result->set_dest_object(dest);
    result->set_dest_port(dest_port);
    result->set_unique(GenerateUniqueID(source_server));
    result->set_payload(payload);

    return result;
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

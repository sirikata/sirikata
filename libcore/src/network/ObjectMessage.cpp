/*  Sirikata
 *  ObjectMessage.cpp
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

#include <sirikata/core/network/ObjectMessage.hpp>

namespace Sirikata {

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

void ObjectMessageDispatcher::dispatchMessage(const Sirikata::Protocol::Object::ObjectMessage& msg) const {
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

Sirikata::Protocol::Object::ObjectMessage* createObjectMessage(ServerID source_server, const UUID& src, uint16 src_port, const UUID& dest, uint16 dest_port, const std::string& payload) {
    Sirikata::Protocol::Object::ObjectMessage* result = new Sirikata::Protocol::Object::ObjectMessage();

    result->set_source_object(src);
    result->set_source_port(src_port);
    result->set_dest_object(dest);
    result->set_dest_port(dest_port);
    result->set_unique(GenerateUniqueID(source_server));
    result->set_payload(payload);

    return result;
}

} // namespace Sirikata

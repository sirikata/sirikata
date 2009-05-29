/*  Sirikata libspace -- Object Connection Services
 *  ObjectConnections.hpp
 *
 *  Copyright (c) 2009, Daniel Reiter Horn
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
#include <space/Platform.hpp>
#include "network/Stream.hpp"
#include "network/StreamListener.hpp"
#include "util/ObjectReference.hpp"
#include "util/RoutableMessageHeader.hpp"
#include "space/ObjectConnections.hpp"
namespace Sirikata {
ObjectConnections::ObjectConnections(Network::StreamListener*listener,
                                     const ObjectReference&registrationServiceIdentifier) {

    mSpace=NULL;
    mListener=listener;
    mRegistrationService=registrationServiceIdentifier;
}

Network::Stream* ObjectConnections::activeConnectionTo(const ObjectReference&ref) {
    std::tr1::unordered_map<ObjectReference,Network::Stream*>::iterator where=mActiveStreams.find(ref);
    if (where==mActiveStreams.end()) 
        return NULL;
    return where->second;
}

Network::Stream* ObjectConnections::temporaryConnectionTo(const UUID&ref) {
    std::map<UUID,Network::Stream*>::iterator where=mTemporaryStreams.find(ref);
    if (where==mTemporaryStreams.end()) 
        return NULL;
    return where->second;
}

void ObjectConnections::promoteConnectionTo(const UUID& tempRef,const ObjectReference&objRef) {
    std::map<UUID,Network::Stream*>::iterator where=mTemporaryStreams.find(tempRef);
    if (where!=mTemporaryStreams.end()) {
        mActiveStreams[objRef]=where->second;
        mTemporaryStreams.erase(where);
    }
}

bool ObjectConnections::forwardMessagesTo(MessageService*ms) {
    if (mSpace==NULL) {
        mSpace=ms;
        return true;
    }
    return false;
}

bool ObjectConnections::endForwardingMessagesTo(MessageService*ms) {
    if (mSpace!=NULL) {
        mSpace=NULL;
        return true;
    }
    return false;
}

void ObjectConnections::processMessage(const ObjectReference*ref,MemoryReference message){
    if (ref&&*ref==mRegistrationService) {
        //FIXME register new message
    }else {
        mSpace->processMessage(ref,message);
    }
}
void ObjectConnections::processMessage(const RoutableMessageHeader&header,
                    MemoryReference message_body){
    if (header.destination_object()==mRegistrationService) {
        //FIXME register new message
    }else {
        mSpace->processMessage(header,message_body);
    }
}
}

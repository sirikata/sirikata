/*  Sirikata Space Physics System -- Manages the creation of proxy objects in a region and ghost volumes around it
 *  Physics.hpp
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

#ifndef SIRIKATA_SPACE_SPACE_PHYSICS_HPP_
#define SIRIKATA_SPACE_SPACE_PHYSICS_HPP_

#include "space/SpaceProxyManager.hpp"
namespace Sirikata {
namespace Space {

class Space;

class Physics : protected SpaceProxyManager, public MessageService {
    class ReplyMessageService:public MessageService {
        MessageService *mSpace; 
        SpaceObjectReference mSenderId;
        uint32 mPort;
    public:
        ReplyMessageService(const SpaceObjectReference &senderId,uint32 port);
        bool forwardMessagesTo(MessageService*);
        bool endForwardingMessagesTo(MessageService*);
        void processMessage(const RoutableMessageHeader&, MemoryReference);        
    }mReplyMessageService;
public:
    Physics(Space*space, Network::IOService*io, const SpaceObjectReference&nodeId, uint32 port);
    ~Physics();
    bool forwardMessagesTo(MessageService*);
    bool endForwardingMessagesTo(MessageService*);
    void processMessage(const RoutableMessageHeader&, MemoryReference);
    ProxyManager*getProxyManager(){
        return this;
    }
};
}
}

#endif

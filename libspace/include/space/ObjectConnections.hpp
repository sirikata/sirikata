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
#ifndef _SIRIKATA_OBJECT_CONNECTIONS_HPP
#define _SIRIKATA_OBJECT_CONNECTIONS_HPP
#include <space/Platform.hpp>
namespace Sirikata {
class SIRIKATA_SPACE_EXPORT ObjectConnections : public MessageService {
    ///Object with active ID's
    std::tr1::unordered_map<ObjectReference,Network::Stream*,ObjectReference::Hasher>mActiveStreams;
    ///Objects in the process of receiving a permanent ID
    std::map<UUID,Network::Stream*>mTemporaryStreams;
    ///to forward messages to
    MessageService * mSpace;
    Network::StreamListener*mListener;
    ObjectReference mRegistrationService;
  public:
    ObjectConnections(Network::StreamListener*listener,                      
                      const ObjectReference&registrationServiceIdentifier);
    ///If there's an active connection to a given object reference
    Network::Stream* activeConnectionTo(const ObjectReference&);
    ///If there's an as-of-yet-unnamed connection to a given object reference
    Network::Stream* temporaryConnectionTo(const UUID&);
    ///A temporary connection has been given a permanent name
    void promoteConnectionTo(const UUID&, const ObjectReference&);
    bool forwardMessagesTo(MessageService*);
    bool endForwardingMessagesTo(MessageService*);
    void processMessage(const ObjectReference*ref,MemoryReference message);
    void processMessage(const RoutableMessageHeader&header,
                        MemoryReference message_body);
    
};
}
#endif

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
#include <network/Stream.hpp>
namespace Sirikata {
class SIRIKATA_SPACE_EXPORT ObjectConnections : public MessageService {
    ///Object with active ID's (ObjectReferences)
    std::tr1::unordered_map<UUID,Network::Stream*,UUID::Hasher>mActiveStreams;
    class TemporaryStreamData {
    public:
        Network::Stream*mStream;
        std::vector<Network::Chunk>mPendingMessages;
    };
    class StreamMapUUID {
        UUID mId;
        bool mConnected;
    public:
        StreamMapUUID() {
            mConnected=false;
        }
        void setId(const UUID&id) {
            mId=id;
        }
        void setConnected(){mConnected=true;}
        void setDisconnected(){mConnected=false;}
        bool connected() {return mConnected;}
        const UUID& uuid() {
            return mId;
        }
        ObjectReference objectReference() {
            return ObjectReference(mId);
        }
    };
    ///Objects in the process of receiving a permanent ID
    std::map<UUID,TemporaryStreamData>mTemporaryStreams;
    std::tr1::unordered_map<Network::Stream*,StreamMapUUID>mStreams;
    ///to forward messages to
    MessageService * mSpace;
    Network::StreamListener*mListener;
    ObjectReference mRegistrationService;
    String mSpaceServiceIntroductionMessage;
    ///processes a message from the RegistrationService: returns true if the object is a new object (false if the object was deleted)
    bool processNewObject(const RoutableMessageHeader&hdr,MemoryReference body_array,ObjectReference&);
    void processExistingObject(const RoutableMessageHeader&hdr,MemoryReference body_array, bool forward);
    void newStreamCallback(Network::Stream*stream,Network::Stream::SetCallbacks&callbacks);
    void connectionCallback(Network::Stream*stream,Network::Stream::ConnectionStatus,const std::string&);
    void bytesReceivedCallback(Network::Stream*stream,const Network::Chunk&chunk);
    void forgeDisconnectionMessage(const ObjectReference&ref);
    void shutdownConnection(const ObjectReference&ref);
  public:
    ObjectConnections(Network::StreamListener*listener,                      
                      const Network::Address &listenAddress,
                      const ObjectReference&registrationServiceIdentifier,
                      const std::string&spaceServiceIntroductionMessage );
    ~ObjectConnections();
    ///If there's an active connection to a given object reference
    Network::Stream* activeConnectionTo(const ObjectReference&);
    ///If there's an as-of-yet-unnamed connection to a given object reference
    Network::Stream* temporaryConnectionTo(const UUID&);
    ///A temporary connection has been given a permanent name
    bool forwardMessagesTo(MessageService*);
    bool endForwardingMessagesTo(MessageService*);
    void processMessage(const ObjectReference*ref,MemoryReference message);
    void processMessage(const RoutableMessageHeader&header,
                        MemoryReference message_body);
    
};
}
#endif

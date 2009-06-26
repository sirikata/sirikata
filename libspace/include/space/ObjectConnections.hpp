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

/**
 * This class holds all the direct object connections out to actual live objects connected to this space node
 * The class is responsible for temporary streams which have not completed registration as well as active streams
 * Temporary streams are saved by a random UUID generated upon connection attempt
 * Permanent streams are generated when the Registration service returns a valid ObjectReference
 */
class SIRIKATA_SPACE_EXPORT ObjectConnections : public MessageService {
    typedef std::vector<Network::Stream*> StreamSet;
    ///Object with active ID's (map from ObjectReference to Stream*)
    typedef std::tr1::unordered_map<UUID,StreamSet,UUID::Hasher>StreamMap;
    StreamMap mActiveStreams;

    /**
     * This class holds data relevant to streams pending object connections
     * FIXME: should not hang onto PendingMessages until Registration request is sent
     *        to limit the buffering space needed to the round trip time from Registration
     */
    class TemporaryStreamData {
    public:
        Network::Stream*mStream;
        size_t mTotalMessageSize;
        std::vector<Network::Chunk>mPendingMessages;
        TemporaryStreamData(){mStream=NULL;mTotalMessageSize=0;}
    };
    /**
     *This class holds whether a given Stream* is connected and the ID (ObjectReference or temp ID) of the Stream*
     */
    class StreamMapUUID {
        UUID mId;
        bool mConnected;
        bool mConnecting;
    public:
        StreamMapUUID() {
            mConnected=false;
            mConnecting=false;
        }
        void setId(const UUID&id) {
            mId=id;
        }
        void setConnecting(){mConnecting=true;}
        void setDoneConnecting(){mConnecting=false;}
        bool isConnecting() const{return mConnecting;}
        void setConnected(){mConnected=true;}
        void setDisconnected(){mConnected=false;}
        bool connected() const{return mConnected;}
        const UUID& uuid() {
            return mId;
        }
        ObjectReference objectReference() {
            return ObjectReference(mId);
        }
    };
    ///Objects in the process of receiving a permanent ID, mapping a UUID to a Stream* and messages pending send
    typedef std::multimap<UUID,TemporaryStreamData> TemporaryStreamMultimap;
    TemporaryStreamMultimap mTemporaryStreams;
    ///Every active stream maps to either a temporary ID in mTemporaryStreams or a permanent ObjectReference in mActiveStreams
    std::tr1::unordered_map<Network::Stream*,StreamMapUUID>mStreams;
    ///to forward messages to
    MessageService * mSpace;
    ///the maximum number of bytes allowed to be pending for a temporary object id
    size_t mPerObjectTemporarySizeMaximum;
    ///the maximum number of messages allowed to be pending for a temporary object id
    size_t mPerObjectTemporaryNumMessagesMaximum;
    ///The listener class which retrieves new connections from object hosts
    Network::StreamListener*mListener;
    ///ObjectRefernece for the Registration service so those messages can be directly sent there instead of waiting on mPendingMessages
    ObjectReference mRegistrationService;
    ///The message that lets users know which services the space supports and on what ObjectReferences
    String mSpaceServiceIntroductionMessage;
    ///processes a message from the RegistrationService: returns true if the object is a new object (false if the object was deleted)
    bool processNewObject(const RoutableMessageHeader&hdr,MemoryReference body_array,ObjectReference&);
    ///processes a message for an object that exists in the Space (i.e. not a temporary object with fake UUID), forwarding message if necessary
    void processExistingObject(const RoutableMessageHeader&hdr,MemoryReference body_array, bool forward);
    ///callback for new streams, giving them a temporary UUID and assigning it into mTemporaryStreams and mStreams
    void newStreamCallback(Network::Stream*stream,Network::Stream::SetCallbacks&callbacks);
    ///callback for disconnection of new streams, to let the RegistrationService know about them
    void connectionCallback(Network::Stream*stream,Network::Stream::ConnectionStatus,const std::string&);
    /**
     * callback for having received bytes, either queueing them to mPendingMessages,
     *                                     delivering them to mActiveConnections
     *                                     or giving up and forwarding them to mSpace in the hopes
     *                                     that they may to a service or a forwader
     */
    void bytesReceivedCallback(Network::Stream*stream,const Network::Chunk&chunk);
    ///makes a Disconnection message for the Registration service in the event a connection should unexpectedly close
    void forgeDisconnectionMessage(const ObjectReference&ref);
    ///actually close a Stream connection to an object.
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
    ///The space needs to register here so that the ObjectConnection knows how to forward messages
    bool forwardMessagesTo(MessageService*);
    ///Upon destruction the space should deregister itself
    bool endForwardingMessagesTo(MessageService*);
    ///Processes a message destined for an Object referenced by either temporary (from registrationService) or permanent (from anyone else) ID ObjectReference
    void processMessage(const ObjectReference*ref,MemoryReference message);
    ///Processes a message destined for an Object referenced by either temporary (from registrationService) or permanent (from anyone else) ID in the header
    void processMessage(const RoutableMessageHeader&header,
                        MemoryReference message_body);
    
};
}
#endif

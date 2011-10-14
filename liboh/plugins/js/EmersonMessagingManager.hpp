// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef __EMERSON_MESSAGING_MANAGER_HPP__
#define __EMERSON_MESSAGING_MANAGER_HPP__

#include <map>
#include <sirikata/core/odp/SST.hpp>
#include <sirikata/core/network/ObjectMessage.hpp>
#include <string>
#include <sstream>
#include <sirikata/core/util/Liveness.hpp>
#include <sirikata/oh/ObjectHostContext.hpp>

namespace Sirikata{
namespace JS{

class EmersonScript;

typedef SST::Stream<SpaceObjectReference> SSTStream;
typedef SSTStream::Ptr SSTStreamPtr;

// NOTE: virtual on Liveness because JSObjectScript also uses it
class EmersonMessagingManager : public virtual Liveness
{
public:
    EmersonMessagingManager(ObjectHostContext* ctx);
    virtual ~EmersonMessagingManager();

    //EmersonScript must know what to do with messages that we receive.
    /*
      Returns true if decoded payload as a scripting communication message,
      false otherwise.

      Payload should be able to be parsed into JS::Proctocol::JSMessage.  If it
      can be, and deserialization is successful, processes the scripting
      message. (via deserializeMsgAndDispatch.)
     */
    virtual bool handleScriptCommRead(const SpaceObjectReference& src, const SpaceObjectReference& dst, const std::string& payload) = 0;


    bool sendScriptCommMessageReliable(const SpaceObjectReference& sender, const SpaceObjectReference& receiver, const String& msg);
    bool sendScriptCommMessageReliable(const SpaceObjectReference& sender, const SpaceObjectReference& receiver, const String& msg, int8 retries);


    void presenceConnected(const SpaceObjectReference& connPresSporef);
    void presenceDisconnected(const SpaceObjectReference& disconnPresSporef);

private:
    // Possibly save the new stream to mStreams for later use. Since both sides
    // might initiate, we always save the stream initiated by the object with
    // smaller if we identify a conflict.
    void setupNewStream(SSTStreamPtr sstStream);
    // Get a saved stream for the given destination object, or NULL if one isn't
    // available.
    SSTStreamPtr getStream(const SpaceObjectReference& pres, const SpaceObjectReference& remote);

    // "Destroy" (i.e. discard reference to) all streams owned by an object
    void clearStreams(const SpaceObjectReference& pres);

    //reading helpers
    void createScriptCommListenerStreamCB(Liveness::Token alive, const SpaceObjectReference& toListenFrom, int err, SSTStreamPtr sstStream);
    void handleIncomingSubstream(Liveness::Token alive, int err, SSTStreamPtr streamPtr);
    void handleScriptCommStreamRead(Liveness::Token alive, SSTStreamPtr sstptr, String* prevdata, uint8* buffer, int length);

    //writing helper
    void scriptCommWriteStreamConnectedCB(Liveness::Token alive, const String& msg, const SpaceObjectReference& sender, const SpaceObjectReference& receiver, int err, SSTStreamPtr streamPtr, int8 retries);

    // Writes a message to a *substream* of the given stream
    void writeMessage(Liveness::Token alive, SSTStreamPtr streamPtr, const String& msg, const SpaceObjectReference& sender, const SpaceObjectReference& receiver, int8 retries);
    void writeMessageSubstream(Liveness::Token alive, int err, SSTStreamPtr subStreamPtr, const String& msg, const SpaceObjectReference& sender, const SpaceObjectReference& receiver, int8 retries);
    void writeData(Liveness::Token alive, SSTStreamPtr streamPtr, const String& msg, const SpaceObjectReference& sender, const SpaceObjectReference& receiver);


    ObjectHostContext* mMainContext;

    //map of existing presences.  value doesn't matter, just want a quick way of
    //checking if particular presences are connected.
    std::map<SpaceObjectReference, bool> allPres;

    // Streams to be reused for sending messages
    typedef std::tr1::unordered_map<SpaceObjectReference, SSTStreamPtr, SpaceObjectReference::Hasher> StreamMap;
    typedef std::tr1::unordered_map<SpaceObjectReference, StreamMap, SpaceObjectReference::Hasher> PresenceStreamMap;
    PresenceStreamMap mStreams;
};

} //end namespace js
} //end namespace sirikata

#endif

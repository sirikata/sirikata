// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include "EmersonMessagingManager.hpp"
#include "JSLogging.hpp"
#include <sirikata/core/network/Frame.hpp>
#include "Protocol_Frame.pbj.hpp"

namespace Sirikata{
namespace JS{

#define EMERSON_RELIABLE_COMMUNICATION_PORT 5

EmersonMessagingManager::EmersonMessagingManager(ObjectHostContext* ctx)
 : mMainContext(ctx)
{
}

EmersonMessagingManager::~EmersonMessagingManager()
{
    // Close all streams, clear out listeners
    for(PresenceStreamMap::iterator pres_it = mStreams.begin(); pres_it != mStreams.end(); pres_it++) {
        // Stop listening for new streams
        const SpaceObjectReference& pres_id = pres_it->first;
        mMainContext->sstConnMgr()->unlisten(
            SST::EndPoint<SpaceObjectReference>(pres_id,EMERSON_RELIABLE_COMMUNICATION_PORT)
        );

        // Stop listening for substreams on the streams we have, close them
        StreamMap& pres_streams = pres_it->second;
        for(StreamMap::iterator it = pres_streams.begin(); it != pres_streams.end(); it++) {
            SSTStreamPtr stream = it->second;
            stream->unlistenSubstream(EMERSON_RELIABLE_COMMUNICATION_PORT);
            stream->close(false);
        }
    }

    mStreams.clear();
}

void EmersonMessagingManager::presenceConnected(const SpaceObjectReference& connPresSporef)
{
    allPres[connPresSporef] = true;

    //create a listener for scripting messages to presence with sporef connPresSporef.
    mMainContext->sstConnMgr()->listen(
        std::tr1::bind(&EmersonMessagingManager::createScriptCommListenerStreamCB,this,
            livenessToken(), connPresSporef,_1,_2
        ),
        SST::EndPoint<SpaceObjectReference>(connPresSporef,EMERSON_RELIABLE_COMMUNICATION_PORT)
    );
}

void EmersonMessagingManager::presenceDisconnected(const SpaceObjectReference& disconnPresSporef)
{
    std::map<SpaceObjectReference,bool>::iterator allPresFinder = allPres.find(disconnPresSporef);
    if (allPresFinder == allPres.end())
    {
        JSLOG(error,"Error in messaging manager: requested a disconnect for a presence that was not already connected.");
        return;
    }
    allPres.erase(allPresFinder);
    clearStreams(disconnPresSporef);
}

void EmersonMessagingManager::setupNewStream(SSTStreamPtr sstStream) {
    // Regardless of whether we want to hold on to this or not, make sure we set
    // it up in case it gets used

    // Listen for substreams, which is where we'll actually receive data. This
    // top level stream goes unused.
    sstStream->listenSubstream(
        EMERSON_RELIABLE_COMMUNICATION_PORT,
        std::tr1::bind(
            &EmersonMessagingManager::handleIncomingSubstream, this,
            livenessToken(), _1, _2
        )
    );

    // Then figure out whether to save it for ourselves
    SpaceObjectReference pres = sstStream->localEndPoint().endPoint;
    SpaceObjectReference sender = sstStream->remoteEndPoint().endPoint;

    // Simple rule for resolving which stream to save.
    bool save_mine = pres < sender;

    StreamMap& pres_streams = mStreams[pres];

    StreamMap::iterator it = pres_streams.find(sender);
    if (it == pres_streams.end() || !it->second->connected() || save_mine) {
        pres_streams[sender] = sstStream;
    }
}

SSTStreamPtr EmersonMessagingManager::getStream(const SpaceObjectReference& pres, const SpaceObjectReference& remote) {
    PresenceStreamMap::iterator pres_it = mStreams.find(pres);
    if (pres_it == mStreams.end()) return SSTStreamPtr();
    StreamMap& pres_streams = pres_it->second;

    StreamMap::iterator it = pres_streams.find(remote);
    if (it != pres_streams.end()) {
        return it->second;
    }
    return SSTStreamPtr();
}

void EmersonMessagingManager::clearStreams(const SpaceObjectReference& pres) {
    PresenceStreamMap::iterator pres_it = mStreams.find(pres);
    if (pres_it != mStreams.end())
        mStreams.erase(pres_it);
}

//Gets executed whenever a new stream connects to presence with sporef toListenFrom.
void EmersonMessagingManager::createScriptCommListenerStreamCB(Liveness::Token alive, const SpaceObjectReference& toListenFrom, int err, SSTStreamPtr sstStream)
{
    if (!alive) return;

    if (err != SST_IMPL_SUCCESS)
    {
        JSLOG(error,"Error in createScriptCommListenerStreamCB.  Stream could not be created.");
        return;
    }
    setupNewStream(sstStream);
}

void EmersonMessagingManager::handleIncomingSubstream(Liveness::Token alive, int err, SSTStreamPtr streamPtr) {
    if (!alive) return;

    if (err != SST_IMPL_SUCCESS) return;

    String* msgBuf = new String();
    streamPtr->registerReadCallback(
        std::tr1::bind(&EmersonMessagingManager::handleScriptCommStreamRead, this,
            livenessToken(), streamPtr, msgBuf, _1, _2)
    );
}


//Gets executed whenever have additional data to read.
void EmersonMessagingManager::handleScriptCommStreamRead(Liveness::Token alive, SSTStreamPtr sstptr, String* prevdata, uint8* buffer, int length)
{
    if (!alive) {
        delete prevdata;
        sstptr->registerReadCallback(0);
        sstptr->close(false);
        return;
    }

    prevdata->append((const char*)buffer, length);

    // We should only ever get one message per substream, so there's no need to
    // loop and we can clean up as soon as we parse a message successfully.
    std::string msg = Network::Frame::parse(*prevdata);

    // If we don't have a full message, just wait for more
    if (msg.empty())
        return;

    //otherwise, try to handle it.
    handleScriptCommRead(sstptr->remoteEndPoint().endPoint, sstptr->localEndPoint().endPoint, msg);


    delete prevdata;
    sstptr->registerReadCallback(0);
    sstptr->close(false);
}


////////////////writing functions.

bool EmersonMessagingManager::sendScriptCommMessageReliable(const SpaceObjectReference& sender, const SpaceObjectReference& receiver, const String& msg) {
    // arbitrary default # of retries
    return sendScriptCommMessageReliable(sender, receiver, msg, 10);
}

bool EmersonMessagingManager::sendScriptCommMessageReliable(const SpaceObjectReference& sender, const SpaceObjectReference& receiver, const String& msg, int8 retries) {
    bool returner = false;

    // If we have a stream
    SSTStreamPtr streamPtr = getStream(sender, receiver);
    if (streamPtr) {
        writeMessage(livenessToken(), streamPtr, msg, sender, receiver, retries);
        return true;
    }

    // Otherwise, start the process of connecting
    returner = mMainContext->sstConnMgr()->connectStream(
        SST::EndPoint<SpaceObjectReference>(sender,0), //local port is random

        //send to receiver's script comm port
        SST::EndPoint<SpaceObjectReference>(receiver,EMERSON_RELIABLE_COMMUNICATION_PORT),

        //what to do when the connection is created.
        std::tr1::bind(
            &EmersonMessagingManager::scriptCommWriteStreamConnectedCB, this,
            livenessToken(),
            msg, sender,receiver, _1, _2, retries
        )
    );

    return returner;
}


void EmersonMessagingManager::scriptCommWriteStreamConnectedCB(Liveness::Token alive, const String& msg, const SpaceObjectReference& sender, const SpaceObjectReference& receiver, int err, SSTStreamPtr streamPtr, int8 retries)
{
    if (!alive) return;

    //if connection failure, just try to re-connect.
    if (err != SST_IMPL_SUCCESS)
    {
        if (retries > 0)
            sendScriptCommMessageReliable(sender, receiver, msg, --retries);
        return;
    }

    setupNewStream(streamPtr);
    writeMessage(livenessToken(), streamPtr, msg, sender,receiver, retries);
}

void EmersonMessagingManager::writeMessage(Liveness::Token alive, SSTStreamPtr streamPtr, const String& msg, const SpaceObjectReference& sender, const SpaceObjectReference& receiver, int8 retries) {
    if (!alive) return;

    // FIXME we're not pushing data directly here because it doesn't seem like
    // you can actually find out how much data was successfully pushed in. what
    // if we have a very large message?
    streamPtr->createChildStream(
        std::tr1::bind(&EmersonMessagingManager::writeMessageSubstream, this, alive, _1, _2, msg, sender, receiver, retries),
        NULL, 0,
        EMERSON_RELIABLE_COMMUNICATION_PORT, EMERSON_RELIABLE_COMMUNICATION_PORT
    );
}

void EmersonMessagingManager::writeMessageSubstream(Liveness::Token alive, int err, SSTStreamPtr subStreamPtr, const String& msg, const SpaceObjectReference& sender, const SpaceObjectReference& receiver, int8 retries) {
    if (!alive) return;

    if (err != SST_IMPL_SUCCESS)
    {
        // Try the whole process over, starting from scratch in case the stream
        // changed
        if (retries > 0)
            sendScriptCommMessageReliable(sender, receiver, msg, --retries);
        return;
    }

    writeData(livenessToken(), subStreamPtr, Network::Frame::write(msg), sender, receiver);
}

void EmersonMessagingManager::writeData(Liveness::Token alive, SSTStreamPtr streamPtr, const String& msg, const SpaceObjectReference& sender, const SpaceObjectReference& receiver) {
    if (!alive) return;

    int bytesWritten = streamPtr->write((uint8*) msg.data(), msg.size());
    //errored out: log message and abort.
    if (bytesWritten == -1)
    {
        JSLOG(error,"Error sending script communication message in scriptCommStreamCallback.  Not sending message.");
        return;
    }

    //check if full message was written.
    if(bytesWritten < (int)msg.size())
    {
        //retry write infinitely
        String restToWrite = msg.substr(bytesWritten);
        mMainContext->mainStrand->post(
            Duration::milliseconds((int64)20),
            std::tr1::bind(&EmersonMessagingManager::writeData, this, livenessToken(), streamPtr, restToWrite, sender, receiver)
        );
        JSLOG(detailed,"More sript data to write to stream.  Queueing future write operation.");
    }
    else
        streamPtr->close(false);
}


} //end namespace js
} //end namespace sirikata

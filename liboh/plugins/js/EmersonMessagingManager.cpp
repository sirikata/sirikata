
#include "EmersonMessagingManager.hpp"
#include "JSLogging.hpp"


namespace Sirikata{
namespace JS{


EmersonMessagingManager::EmersonMessagingManager(Network::IOService* ios)
 : retryTimer(Network::IOTimer::create(ios))
{
}

EmersonMessagingManager::~EmersonMessagingManager()
{
}

void EmersonMessagingManager::presenceConnected(const SpaceObjectReference& connPresSporef)
{
    allPres[connPresSporef] = true;
    
    //create a listener for scripting messages to presence with sporef connPresSporef.
    SSTStream::listen(
        std::tr1::bind(&EmersonMessagingManager::createScriptCommListenerStreamCB,this,connPresSporef,_1,_2),
        EndPoint<SpaceObjectReference>(connPresSporef,OBJECT_SCRIPT_COMMUNICATION_PORT));
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
}


//Gets executed whenever a new stream connects to presence with sporef toListenFrom.
void EmersonMessagingManager::createScriptCommListenerStreamCB(const SpaceObjectReference& toListenFrom, int err, SSTStreamPtr sstStream)
{
    if (err != SST_IMPL_SUCCESS)
    {
        JSLOG(error,"Error in createScriptCommListenerStreamCB.  Stream could not be created.");
        return;
    }

    //on this stream, listen for additional data.
    sstStream->registerReadCallback(
        std::tr1::bind(&EmersonMessagingManager::handleScriptCommStreamRead, this, toListenFrom, sstStream, new std::stringstream(), _1, _2) );
}

//Gets executed whenever have additional data to read.
void EmersonMessagingManager::handleScriptCommStreamRead(const SpaceObjectReference& spaceobj, SSTStreamPtr sstptr, std::stringstream* prevdata, uint8* buffer, int length)
{
    prevdata->write((const char*)buffer, length);

    std::cout<<"\n\nDEBUG: this is the length of the message that I received: "<<length<<"\n";
    
    //if do not have an object script, or our objectscript processed the
    //scripting communication message, then perform cleanup.
    if (handleScriptCommRead(sstptr->remoteEndPoint().endPoint,spaceobj,prevdata->str()))
    {
        // FIXME we should be getting a callback on stream close instead of
        // relying on this parsing as an indicator
        delete prevdata;
        // Clear out callback so we aren't responsible for any remaining
        // references to sstptr
        sstptr->registerReadCallback(0);
    }
}


////////////////writing functions.

bool EmersonMessagingManager::sendScriptCommMessageReliable(const SpaceObjectReference& sender, const SpaceObjectReference& receiver, const String& msg)
{
    bool returner = false;
    returner = SSTStream::connectStream(
        EndPoint<SpaceObjectReference>(sender,0), //local port is random

        //send to receiver's script comm port
        EndPoint<SpaceObjectReference>(receiver,OBJECT_SCRIPT_COMMUNICATION_PORT), 

        //what to do when the connection is created.
        std::tr1::bind(
            &EmersonMessagingManager::scriptCommWriteStreamConnectedCB, this,
            msg, sender,receiver, _1, _2
        ) 
    );

    return returner;
}


void EmersonMessagingManager::scriptCommWriteStreamConnectedCB(const String& msg, const SpaceObjectReference& sender, const SpaceObjectReference& receiver, int err, SSTStreamPtr streamPtr)
{
    //if connection failure, just try to re-connect.
    if (err != SST_IMPL_SUCCESS)
    {
        SSTStream::connectStream(
            EndPoint<SpaceObjectReference>(sender,0), //local port is random
            EndPoint<SpaceObjectReference>(receiver,OBJECT_SCRIPT_COMMUNICATION_PORT), //send to
                                                                    //receiver's
                                                                    //script
                                                                    //comm port
            std::tr1::bind(
                &EmersonMessagingManager::scriptCommWriteStreamConnectedCB, this,
                msg, sender, receiver, _1, _2
            )  //what to do when the connection is created.
        );
        return;
    }

    int bytesWritten = streamPtr->write((uint8*) msg.data(), msg.size());

    //error code.  just return, aborting write.
    if (bytesWritten == -1)
    {
        JSLOG(error,"Error sending script communication message in scriptCommStreamCallback.  Not sending message.");
        return;
    }
    
    //check if full message was written.
    if(bytesWritten < msg.size())
    {
        //retry write infinitely
        String restToWrite = msg.substr(msg.size());
        retryTimer->wait(
            Duration::milliseconds((int64)20),
            std::tr1::bind(&EmersonMessagingManager::scriptCommWriteStreamConnectedCB, this, restToWrite,sender,receiver,SST_IMPL_SUCCESS,streamPtr)
        );
        JSLOG(detailed,"More sript data to write to stream.  Queueing future write operation.");
    }
    
}



} //end namespace js
} //end namespace sirikata

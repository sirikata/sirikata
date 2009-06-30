/*  Sirikata Network Utilities
 *  MultiplexedSocket.cpp
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
#include "util/Standard.hh"
#include "TCPDefinitions.hpp"
#include "Stream.hpp"
#include "TCPStream.hpp"
#include "util/ThreadSafeQueue.hpp"
#include "ASIOSocketWrapper.hpp"
#include "MultiplexedSocket.hpp"
#include "ASIOConnectAndHandshake.hpp"
#include "TCPSetCallbacks.hpp"

namespace Sirikata { namespace Network {


boost::mutex MultiplexedSocket::sConnectingMutex; 


void triggerMultiplexedConnectionError(MultiplexedSocket*socket,ASIOSocketWrapper*wrapper,const boost::system::error_code &error){
    socket->hostDisconnectedCallback(wrapper,error);
}

void MultiplexedSocket::ioReactorThreadCommitCallback(StreamIDCallbackPair& newcallback){
    if (newcallback.mCallback==NULL) {
        //make sure that a new substream callback won't be sent for outstanding closing streams
        mOneSidedClosingStreams.insert(newcallback.mID);
        CallbackMap::iterator where=mCallbacks.find(newcallback.mID);
        if (where!=mCallbacks.end()) {
            delete where->second;
            mCallbacks.erase(where);
        }else {
            assert("ERROR in finding callback to erase for stream ID"&&false);
        }
    }else {
        mCallbacks.insert(newcallback.pair());
    }
}

bool MultiplexedSocket::CommitCallbacks(std::deque<StreamIDCallbackPair> &registration, SocketConnectionPhase status, bool setConnectedStatus) {
    assertThreadGroup(*static_cast<const ThreadIdCheck*>(this));
    bool statusChanged=false;
    if (setConnectedStatus||!mCallbackRegistration.empty()) {
        if (status==CONNECTED) {
            //do a little house cleaning and empty as many new requests as possible
            std::vector<RawRequest> newRequests;            
            {
                boost::lock_guard<boost::mutex> connecting_mutex(sConnectingMutex);
                newRequests.swap(mNewRequests);
            }
            for (size_t i=0,ie=newRequests.size();i<ie;++i) {
                sendBytesNow(getSharedPtr(),newRequests[i]);
            }
            
        }
        boost::lock_guard<boost::mutex> connecting_mutex(sConnectingMutex);
        statusChanged=(status!=mSocketConnectionPhase);
        if (setConnectedStatus) {
            if (status!=CONNECTED) {
                mSocketConnectionPhase=status;
            } else {
                mSocketConnectionPhase=WAITCONNECTING;
                for (size_t i=0,ie=mNewRequests.size();i<ie;++i) {
                    sendBytesNow(getSharedPtr(),mNewRequests[i]);
                }
                mNewRequests.clear();
                mSocketConnectionPhase=CONNECTED;
            }
        }
        bool other_registrations=registration.empty();
        mCallbackRegistration.swap(registration);
    }
    while (!registration.empty()) {
        ioReactorThreadCommitCallback(registration.front());
        registration.pop_front();
    }
    return statusChanged;
}

size_t MultiplexedSocket::leastBusyStream() {
        ///FIXME look at bytes to be sent or recently used, etc
        return rand()%mSockets.size();
 }
float MultiplexedSocket::dropChance(const Chunk*data,size_t whichStream) {
    return .25;
}

void MultiplexedSocket::sendBytesNow(const std::tr1::shared_ptr<MultiplexedSocket>&thus,const RawRequest&data) {
    TCPSSTLOG(this,"sendnow",&*data.data->begin(),data.data->size(),false);
    TCPSSTLOG(this,"sendnow","\n",1,false);
    static Stream::StreamID::Hasher hasher;
    if (data.originStream==Stream::StreamID()) {
        unsigned int socket_size=(unsigned int)thus->mSockets.size();
        for(unsigned int i=1;i<socket_size;++i) {                
            thus->mSockets[i].rawSend(thus,new Chunk(*data.data));
        }
        thus->mSockets[0].rawSend(thus,data.data);
    }else {
        size_t whichStream=data.unordered?thus->leastBusyStream():hasher(data.originStream)%thus->mSockets.size();
        if (data.unreliable==false||rand()/(float)RAND_MAX>thus->dropChance(data.data,whichStream)) {
            thus->mSockets[whichStream].rawSend(thus,data.data);
        }        
    }
}


void MultiplexedSocket::closeStream(const std::tr1::shared_ptr<MultiplexedSocket>&thus,const Stream::StreamID&sid,TCPStream::TCPStreamControlCodes code) {
    RawRequest closeRequest;
    closeRequest.originStream=Stream::StreamID();//control packet
    closeRequest.unordered=false;
    closeRequest.unreliable=false;
    closeRequest.data=ASIOSocketWrapper::constructControlPacket(code,sid);
    sendBytes(thus,closeRequest);
}


void MultiplexedSocket::sendBytes(const std::tr1::shared_ptr<MultiplexedSocket>&thus,const RawRequest&data) {
    if (thus->mSocketConnectionPhase==CONNECTED) {
        sendBytesNow(thus,data);
    }else {
        bool lockCheckConnected=false;
        {
            boost::lock_guard<boost::mutex> connectingMutex(sConnectingMutex);
            if (thus->mSocketConnectionPhase==CONNECTED) {
                lockCheckConnected=true;
            }else if(thus->mSocketConnectionPhase==DISCONNECTED) {
                //retval=false;
                //FIXME is this the correct thing to do?
                TCPSSTLOG(this,"sendnvr",&*data.data->begin(),data.data->size(),false);                
                TCPSSTLOG(this,"sendnvr","\n",1,false);
                thus->mNewRequests.push_back(data);
            }else {
                //with the connectionMutex acquired, no socket is allowed to be in the mSocketConnectionPhase
                assert(thus->mSocketConnectionPhase==PRECONNECTION);
                TCPSSTLOG(this,"sendl8r",&*data.data->begin(),data.data->size(),false);
                TCPSSTLOG(this,"sendl8r","\n",1,false);
                thus->mNewRequests.push_back(data);
            }
        }
        if (lockCheckConnected) {
            sendBytesNow(thus,data);
        }
    }
}

MultiplexedSocket::SocketConnectionPhase MultiplexedSocket::addCallbacks(const Stream::StreamID&sid, 
                                                                         TCPStream::Callbacks* cb) {
    boost::lock_guard<boost::mutex> connectingMutex(sConnectingMutex);
    mCallbackRegistration.push_back(StreamIDCallbackPair(sid,cb));
    return mSocketConnectionPhase;
}


Stream::StreamID MultiplexedSocket::getNewID() {
    if (!mFreeStreamIDs.probablyEmpty()) {
        Stream::StreamID retval;
        if (mFreeStreamIDs.pop(retval))
            return retval;
    }
    unsigned int retval=mHighestStreamID+=2;
    assert(retval>1);
    return Stream::StreamID(retval);
}
MultiplexedSocket::MultiplexedSocket(IOService*io, const Stream::SubstreamCallback&substreamCallback):ThreadIdCheck(ThreadId::registerThreadGroup(NULL)),mIO(io),mNewSubstreamCallback(substreamCallback),mHighestStreamID(1) {
    mSocketConnectionPhase=PRECONNECTION;
}
MultiplexedSocket::MultiplexedSocket(IOService*io,const UUID&uuid,const std::vector<TCPSocket*>&sockets, const Stream::SubstreamCallback &substreamCallback)
    :ThreadIdCheck(ThreadId::registerThreadGroup(NULL)),mIO(io),
     mNewSubstreamCallback(substreamCallback),
     mHighestStreamID(0) {
    mSocketConnectionPhase=PRECONNECTION;
    for (unsigned int i=0;i<(unsigned int)sockets.size();++i) {
        mSockets.push_back(ASIOSocketWrapper(sockets[i]));
    }
}
void MultiplexedSocket::sendAllProtocolHeaders(const std::tr1::shared_ptr<MultiplexedSocket>&thus,const UUID&syncedUUID) {
    unsigned int numSockets=(unsigned int)thus->mSockets.size();
    for (std::vector<ASIOSocketWrapper>::iterator i=thus->mSockets.begin(),ie=thus->mSockets.end();i!=ie;++i) {
        i->sendProtocolHeader(thus,syncedUUID,numSockets);
    }
    boost::lock_guard<boost::mutex> connectingMutex(sConnectingMutex);
    thus->mSocketConnectionPhase=CONNECTED;
    for (unsigned int i=0,ie=thus->mSockets.size();i!=ie;++i) {
        MakeASIOReadBuffer(thus,i);
    }
    assert (thus->mNewRequests.size()==0);//would otherwise need to empty out new requests--but no one should have a reference to us here
}
///erase all sockets and callbacks since the refcount is now zero;
MultiplexedSocket::~MultiplexedSocket() {        
    Stream::SubstreamCallback callbackToBeDeleted=mNewSubstreamCallback;
    mNewSubstreamCallback=&Stream::ignoreSubstreamCallback;
    TCPSetCallbacks setCallbackFunctor(this,NULL);        
    callbackToBeDeleted(NULL,setCallbackFunctor);
    for (unsigned int i=0;i<(unsigned int)mSockets.size();++i){
        mSockets[i].shutdownAndClose();
    }        
    boost::lock_guard<boost::mutex> connecting_mutex(sConnectingMutex);        
    for (unsigned int i=0;i<(unsigned int)mSockets.size();++i){
        mSockets[i].destroySocket();
    }
    mSockets.clear();
    
    while (!mCallbackRegistration.empty()){
        delete mCallbackRegistration.front().mCallback;
        mCallbackRegistration.pop_front();
    }
    for (size_t i=0;i<mNewRequests.size();++i) {
        delete mNewRequests[i].data;
    }
    mNewRequests.clear();
    while(!mCallbacks.empty()) {
        delete mCallbacks.begin()->second;
        mCallbacks.erase(mCallbacks.begin());
    }    
}

void MultiplexedSocket::shutDownClosedStream(unsigned int controlCode,const Stream::StreamID &id) {
    if (controlCode==TCPStream::TCPStreamCloseStream){
        std::deque<StreamIDCallbackPair> registrations;
        CommitCallbacks(registrations,CONNECTED,false);
        CallbackMap::iterator where=mCallbacks.find(id);
        if (where!=mCallbacks.end()) {
            std::tr1::shared_ptr<AtomicValue<int> > sendStatus=where->second->mSendStatus.lock();
            if (sendStatus) {
                TCPStream::closeSendStatus(*sendStatus);
            }
            where->second->mConnectionCallback(Stream::Disconnected,"Remote Host Disconnected");
            
            CommitCallbacks(registrations,CONNECTED,false);//just in case stream committed new callbacks during callback
            where=mCallbacks.find(id);
            delete where->second;
            mCallbacks.erase(where);
        }                                    
    }
    std::tr1::unordered_set<Stream::StreamID>::iterator where=mOneSidedClosingStreams.find(id);
    if (where!=mOneSidedClosingStreams.end()) {
        mOneSidedClosingStreams.erase(where);
    }
    if (id.odd()==((mHighestStreamID.read()&1)?true:false)) {
        mFreeStreamIDs.push(id);
    }
}
void MultiplexedSocket::receiveFullChunk(unsigned int whichSocket, Stream::StreamID id,const Chunk&newChunk){
    if (id==Stream::StreamID()) {//control packet
        if(newChunk.size()) {
            unsigned int controlCode=*newChunk.begin();
            switch (controlCode) {
              case TCPStream::TCPStreamCloseStream:
              case TCPStream::TCPStreamAckCloseStream:
                if (newChunk.size()>1) {
                    unsigned int avail_len=newChunk.size()-1;
                    id.unserialize((const uint8*)&(newChunk[1]),avail_len);
                    if (avail_len+1>newChunk.size()) {
                        SILOG(tcpsst,warning,"Control Chunk too short");
                    }
                }
                if (id!=Stream::StreamID()) {
                    std::tr1::unordered_map<Stream::StreamID,unsigned int>::iterator where=mAckedClosingStreams.find(id);
                    if (where!=mAckedClosingStreams.end()){
                        where->second++;
                        int how_much=where->second;
                        if (where->second==mSockets.size()) {
                            mAckedClosingStreams.erase(where);        
                            shutDownClosedStream(controlCode,id);
                            if (controlCode==TCPStream::TCPStreamCloseStream) {
                                closeStream(getSharedPtr(),id,TCPStream::TCPStreamAckCloseStream);
                            }
                        }
                    }else{
                        if (mSockets.size()==1) {
                            shutDownClosedStream(controlCode,id);
                            if (controlCode==TCPStream::TCPStreamCloseStream) {
                                closeStream(getSharedPtr(),id,TCPStream::TCPStreamAckCloseStream);
                            }
                        }else {
                            mAckedClosingStreams[id]=1;
                        }
                    }
                }
                break;
              default:
                break;
            }
        }
    }else {
        std::deque<StreamIDCallbackPair> registrations;
        CommitCallbacks(registrations,CONNECTED,false);
        CallbackMap::iterator where=mCallbacks.find(id);
        if (where!=mCallbacks.end()) {
            where->second->mBytesReceivedCallback(newChunk);
        }else if (mOneSidedClosingStreams.find(id)==mOneSidedClosingStreams.end()) {
            //new substream
            TCPStream*newStream=new TCPStream(getSharedPtr(),id);
            TCPSetCallbacks setCallbackFunctor(this,newStream);
            mNewSubstreamCallback(newStream,setCallbackFunctor);
            if (setCallbackFunctor.mCallbacks != NULL) {
                CommitCallbacks(registrations,CONNECTED,false);//make sure bytes are received
                setCallbackFunctor.mCallbacks->mBytesReceivedCallback(newChunk);
            }else {
                closeStream(getSharedPtr(),id);
            }
        }else {
            //IGNORED MESSAGE
        }
    }
}
void MultiplexedSocket::connectionFailureOrSuccessCallback(SocketConnectionPhase status, Stream::ConnectionStatus reportedProblem, const std::string&errorMessage) {
    Stream::ConnectionStatus stat=reportedProblem;
    std::deque<StreamIDCallbackPair> registrations;
    bool actuallyDoSend=CommitCallbacks(registrations,status,true);
    if (actuallyDoSend) {
        for (CallbackMap::iterator i=mCallbacks.begin(),ie=mCallbacks.end();i!=ie;++i) {
            i->second->mConnectionCallback(stat,errorMessage);
        }
    }else {
        //SILOG(tcpsst,debug,"Did not call callbacks because callback message already sent for "<<errorMessage);
    }
}
void MultiplexedSocket::connectionFailedCallback(const std::string& error) {
    
    connectionFailureOrSuccessCallback(DISCONNECTED,Stream::ConnectionFailed,error);
}
void MultiplexedSocket::connectionFailedCallback(unsigned int whichSocket, const std::string& error) {
    connectionFailedCallback(error);
    //FIXME do something with the socket specifically that failed.
}

void MultiplexedSocket::hostDisconnectedCallback(const std::string& error) {
    
    connectionFailureOrSuccessCallback(DISCONNECTED,Stream::Disconnected,error);
}
void MultiplexedSocket::hostDisconnectedCallback(unsigned int whichSocket, const std::string& error) {
    hostDisconnectedCallback(error);
    //FIXME do something with the socket specifically that failed.
}


void MultiplexedSocket::prepareConnect(unsigned int numSockets) {
    mSocketConnectionPhase=PRECONNECTION;
    unsigned int oldSize=(unsigned int)mSockets.size();
    if (numSockets>mSockets.size()) {
        mSockets.resize(numSockets);
        for (unsigned int i=oldSize;i<numSockets;++i) {
            mSockets[i].createSocket(getASIOService());
        }
    }
}
void MultiplexedSocket::connect(const Address&address, unsigned int numSockets) {
    prepareConnect(numSockets);
    std::tr1::shared_ptr<ASIOConnectAndHandshake> 
        headerCheck(new ASIOConnectAndHandshake(getSharedPtr(),
                                                UUID::random()));
    //will notify connectionFailureOrSuccessCallback when resolved
    ASIOConnectAndHandshake::connect(headerCheck,address);
}
} }

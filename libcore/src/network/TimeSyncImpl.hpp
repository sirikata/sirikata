/*  Sirikata Network Utilities
 *  TimeSyncImpl.hpp
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

#ifndef SIRIKATA_TimeSyncImpl_HPP__
#define SIRIKATA_TimeSyncImpl_HPP__
#include "network/IOServiceFactory.hpp"
namespace Sirikata {
namespace Network {
template <class WeakRef> class TimeSyncImpl : public TimeSync {
    WeakRef mParent;
    IOService*mWaitService;
    Network::Stream *mStream;
    Duration mRetryDelay;
    int mNumAverage;
    int mNumParallel;
    uint64 mSyncRound;
    std::vector<Protocol::TimeSync>mSamples;
    void sendParallelPackets() {
        RoutableMessageHeader rmh;
        rmh.set_destination_object(ObjectReference::spaceServiceID());
        rmh.set_destination_port(Services::TIMESYNC);
        std::string syncstr;
        rmh.SerializeToString(&syncstr);
        Protocol::TimeSync sync;
        sync.set_sync_round(mSyncRound);
        sync.set_client_time(Time::now());
        sync.AppendToString(&syncstr);
        for (int i=0;i<mNumParallel;++i) {
            mStream->send(MemoryReference(syncstr),Unreliable);
        }
    }
    template <class Parent> void internalBytesReceived(const Parent&parent, const Network::Chunk &data) {
        if (parent&&data.size()) {
            Protocol::TimeSync sync;
            sync.ParseFromArray(&data[0],data.size());
            if (sync.has_sync_round()&&sync.sync_round()==mSyncRound&&sync.has_server_time()&&sync.has_client_time()) {
                mSamples.push_back(sync);
                incrementSyncRound();
                if (mSamples.size()>=(size_t)mNumAverage) {
                    calculateDuration();
                }else {
                    sendParallelPackets();
                }
            }

        }
    }
    void calculateDuration() {
        std::vector<Time> mOffsets(mSamples.size(),Time::epoch());
        Time now(Time::now());
        for (size_t i=0,ie=mOffsets.size();i<ie;++i) {
            mOffsets[i]=mSamples[i].server_time()-(now-mSamples[i].client_time())*.5;
        }
        std::sort(mOffsets.begin(),mOffsets.end());
        for (size_t i=0,ie=mOffsets.size();i<ie;++i) {
            SILOGNOCR(objecthost,debug,mOffsets[i]-now<<',');
        }
        Duration oldOffset=mOffset;
        mOffset=mOffsets[mOffsets.size()/2]-now;
        SILOG(objecthost,debug,"Old offset was "<<oldOffset<<" new offset is "<<mOffset);
    }
    void incrementSyncRound(){
        ++mSyncRound;
        Duration mMaxDelayedPacket=Duration::seconds(128);
        if (mRetryDelay*((double)mSyncRound/(double)mNumAverage)>mMaxDelayedPacket){
            mSyncRound=0;
        }
    }
protected:
    void internalStartSync() {//to prevent toplevel class from being instantiated
        incrementSyncRound();
        mSamples.resize(0);
        sendParallelPackets();
    }


    
    static void startSync(const std::tr1::weak_ptr<TimeSyncImpl<WeakRef> >&weak_thus, const Duration&delay) {
        std::tr1::shared_ptr<TimeSyncImpl<WeakRef> >thus (weak_thus.lock());
        if (thus&&delay==thus->mRetryDelay) {
            thus->internalStartSync();
            std::tr1::function<void(std::tr1::weak_ptr<TimeSyncImpl<WeakRef> > weak_thus, Duration delay)> myfunc(&TimeSyncImpl<WeakRef>::startSync);
            IOServiceFactory::dispatchServiceMessage(thus->mWaitService,delay,std::tr1::bind(myfunc,weak_thus,delay));
        }
    }
public:
    TimeSyncImpl(const WeakRef& parent, IOService *waitService){
        mParent=parent;
        mWaitService=waitService;
        mStream=NULL;
        mNumParallel=0;
        mNumAverage=0;
        mRetryDelay=Duration::seconds(0);

        mSyncRound=0;        
    }
    static void bytesReceived(const std::tr1::weak_ptr<TimeSyncImpl<WeakRef> >&weak_thus, const Network::Chunk&data) {
        std::tr1::shared_ptr<TimeSyncImpl<WeakRef> >thus (weak_thus.lock());
        if (thus) {
            thus->internalBytesReceived(thus->mParent.lock(),data);
        }
    }
    void go(const std::tr1::shared_ptr<TimeSyncImpl<WeakRef> >&thus, int numParallel, int numAverage, const Duration&delay, Network::Stream*topLevelStream){
        thus->mStream=topLevelStream;
        thus->mNumParallel=numParallel;
        thus->mNumAverage=numAverage;
        thus->mRetryDelay=delay;
        startSync(thus,delay);
    }
    
    


};
}
}
#endif

/*  Sirikata
 *  FairServerMessageQueue.hpp
 *
 *  Copyright (c) 2010, Daniel Reiter Horn
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

#ifndef _SIRIKATA_FAIRSENDQUEUE_HPP
#define _SIRIKATA_FAIRSENDQUEUE_HPP

#include <sirikata/core/queue/FairQueue.hpp>
#include "ServerMessageQueue.hpp"

namespace Sirikata {
class FairServerMessageQueue:public ServerMessageQueue {
protected:
    struct SenderAdapterQueue {
        SenderAdapterQueue(Sender* sender, ServerID sid);
        Message* front();
        Message* pop();
        bool empty();
      private:
        Sender* mSender;
        ServerID mDestServer;
        Message* mFront;
    };

    typedef FairQueue<Message, ServerID, SenderAdapterQueue> FairSendQueue;
    FairSendQueue mServerQueues;

    Sirikata::AtomicValue<bool> mServiceScheduled;

    uint64 mStoppedBlocked;
    uint64 mStoppedUnderflow;

    // These must be recursive because networkReadyToSend callbacks may or may
    // not be triggered by this code.
    mutable boost::recursive_mutex mMutex;
    typedef boost::lock_guard<boost::recursive_mutex> MutexLock;
  public:
    FairServerMessageQueue(SpaceContext* ctx, SpaceNetwork* net, Sender* sender);
    ~FairServerMessageQueue();

  protected:
    // Must be thread safe:

    // Public ServerMessageQueue interface
    virtual void messageReady(ServerID sid);
    // SpaceNetwork::SendListener Interface
    virtual void networkReadyToSend(const ServerID& from);

    // Should always be happening inside ServerMessageQueue thread

    // ServerMessageReceiver Protected (Implementation) Interface
    virtual void handleUpdateReceiverStats(ServerID sid, double total_weight, double used_weight);

    // Internal methods
    void addInputQueue(ServerID sid, float weight);
    void removeInputQueue(ServerID sid);

    // Get average weight over all queues.  Used when we don't have a weight for
    // a new input queue yet.
    float getAverageServerWeight() const;

    void enableDownstream(ServerID sid);
    void disableDownstream(ServerID sid);

    // Schedules servicing to occur, but only if it isn't already currently scheduled.
    void scheduleServicing();
    // Services the queue, will be called in response to network ready events
    // and message ready events since one of those conditions must have changed
    // in order to make any additional progress.
    void service();
};

}

#endif

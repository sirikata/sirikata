/*  Sirikata Transfer -- Content Transfer management system
 *  TransferPool.hpp
 *
 *  Copyright (c) 2010, Jeff Terrace
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
/*  Created on: Jan 20th, 2010 */

#ifndef SIRIKATA_TransferPool_HPP__
#define SIRIKATA_TransferPool_HPP__

#include <sirikata/core/transfer/Defs.hpp>
#include <sirikata/core/queue/ThreadSafeQueue.hpp>
#include <sirikata/core/transfer/TransferRequest.hpp>

namespace Sirikata {
namespace Transfer {

/*
 * Abstract class for providing an algorithm for aggregating priorities from
 * one or more clients for a request
 */
class PriorityAggregationAlgorithm {

public:
    // Return an aggregated priority given a list of priorities
    virtual Priority aggregate(
        const std::vector<Priority> &) const = 0;

    //Return an aggregated priority given the list of priorities
    virtual Priority aggregate(
        const std::map<std::string, TransferRequestPtr> &) const = 0;

    virtual Priority aggregate(
        const std::vector<TransferRequestPtr>&) const = 0;

    virtual ~PriorityAggregationAlgorithm() {
    }
};

/** Pools are the conduits for requests to get into the
 *  TransferMediator for processing. Pools interact carefully with the
 *  TransferMediator, which has stricter rules (e.g. only one
 *  outstanding request for a resource at a time with a single
 *  priority), but provide more convenient interfaces to the
 *  user. Implementations might do very little, almost directly
 *  passing requests on, or might provide an additional layer of
 *  aggregation of requests and multiplexing of callbacks.  This
 *  intermediate layer allows individual requests to remain simple
 *  but provides a layer for coordination (e.g. for aggregation).
 */
class TransferPool {
public:
    virtual ~TransferPool() {}

    /// Returns client identifier
    inline const std::string& getClientID() const {
        return mClientID;
    }

    /// Puts a request into the pool
    virtual void addRequest(TransferRequestPtr req) = 0;
    /// Updates priority of a request in the pool
    virtual void updatePriority(TransferRequestPtr req, Priority p) = 0;
    /// Updates priority of a request in the pool
    virtual void deleteRequest(TransferRequestPtr req) = 0;

protected:
    // Friend in TransferMediator so it can construct, call getRequest
    friend class TransferMediator;

    TransferPool(const std::string& clientID)
     : mClientID(clientID)
    {}

    virtual TransferRequestPtr getRequest() = 0;

    // Utility methods because they require being friended by
    // TransferRequest but that doesn't extend to subclasses
    void setRequestClientID(TransferRequestPtr req) {
        req->setClientID(mClientID);
    }
    void setRequestPriority(TransferRequestPtr req, Priority p) {
        req->setPriority(p);
    }
    void setRequestDeletion(TransferRequestPtr req) {
        req->setDeletion();
    }

    const std::string mClientID;
};
typedef std::tr1::shared_ptr<TransferPool> TransferPoolPtr;

/** Simplest implementation of TransferPool. The user *must only have
 *  one outstanding request for any given resource at a time*.
 */
class SimpleTransferPool : public TransferPool {
public:
    virtual ~SimpleTransferPool() {}

    //Puts a request into the pool
    virtual void addRequest(TransferRequestPtr req) {
        if (req)
            setRequestClientID(req);
        mDeltaQueue.push(req);
    }

    //Updates priority of a request in the pool
    virtual void updatePriority(TransferRequestPtr req, Priority p) {
        setRequestPriority(req, p);
        mDeltaQueue.push(req);
    }

    //Updates priority of a request in the pool
    inline void deleteRequest(TransferRequestPtr req) {
        setRequestDeletion(req);
        mDeltaQueue.push(req);
    }

private:
    // Friend in TransferMediator so it can construct, call getRequest
    friend class TransferMediator;

    ThreadSafeQueue<TransferRequestPtr> mDeltaQueue;

    SimpleTransferPool(const std::string &clientID)
     : TransferPool(clientID)
    {
    }

    //Returns an item from the pool. Blocks if pool is empty.
    inline std::tr1::shared_ptr<TransferRequest> getRequest() {
        std::tr1::shared_ptr<TransferRequest> retval;
        mDeltaQueue.blockingPop(retval);
        return retval;
    }
};

}
}

#endif

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

#include <sirikata/core/task/WorkQueue.hpp>
#include <sirikata/core/queue/ThreadSafeQueue.hpp>
#include <sirikata/core/transfer/TransferData.hpp>
#include "RemoteFileMetadata.hpp"
#include "URI.hpp"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>


namespace Sirikata {
namespace Transfer {

class TransferRequest;
typedef std::tr1::shared_ptr<TransferRequest> TransferRequestPtr;

/*
 * Base class for a request going into the transfer pool to set
 * the priority of a request
 */
class SIRIKATA_EXPORT TransferRequest {

public:
    typedef std::tr1::function<void()> ExecuteFinished;
    typedef float PriorityType;

	//Return the priority of the request
	inline PriorityType getPriority() const {
		return mPriority;
	}

	//Returns true if this request is for deletion
	inline bool isDeletionRequest() const {
	    return mDeletionRequest;
	}

	/// Get an identifier for the data referred to by this
	/// TransferRequest. The identifier is not unique for each
	/// TransferRequest. Instead, it identifies the asset data: if two
	/// identifiers are equal, they refer to the same data. (Two different
	/// identifiers may *ultimately* refer to the same data because two
	/// names could point to the same underlying hash).
	virtual const std::string& getIdentifier() const = 0;

	inline const std::string& getClientID() const {
		return mClientID;
	}

	virtual void execute(std::tr1::shared_ptr<TransferRequest> req, ExecuteFinished cb) = 0;

    virtual void notifyCaller(std::tr1::shared_ptr<TransferRequest>) = 0;

	virtual ~TransferRequest() {}

	friend class TransferPool;

protected:
	inline void setClientID(const std::string& clientID) {
		mClientID = clientID;
	}

    //Change the priority of the request
    inline void setPriority(PriorityType p) {
        mPriority = p;
    }

    //Change the priority of the request
    inline void setDeletion() {
        mDeletionRequest = true;
    }

	PriorityType mPriority;
	std::string mClientID;
	bool mDeletionRequest;

};

/*
 * Handles requests for metadata of a file when all you have is the URI
 */

class SIRIKATA_EXPORT MetadataRequest: public TransferRequest {

public:
    typedef std::tr1::function<void(
            std::tr1::shared_ptr<MetadataRequest> request,
            std::tr1::shared_ptr<RemoteFileMetadata> response)> MetadataCallback;

    MetadataRequest(const URI &uri, PriorityType priority, MetadataCallback cb) :
     mURI(uri), mCallback(cb) {
        mPriority = priority;
        mDeletionRequest = false;
        mID = uri.toString();
    }

    inline const std::string &getIdentifier() const {
        return mID;
    }

    inline const URI& getURI() {
        return mURI;
    }

    void execute(std::tr1::shared_ptr<TransferRequest> req, ExecuteFinished cb);

    inline void notifyCaller(std::tr1::shared_ptr<TransferRequest> from) {
        std::tr1::shared_ptr<MetadataRequest> fromC =
                std::tr1::static_pointer_cast<MetadataRequest, TransferRequest>(from);
        mCallback(fromC, fromC->mRemoteFileMetadata);
    }
    inline void notifyCaller(TransferRequestPtr from, RemoteFileMetadataPtr data) {
        std::tr1::shared_ptr<MetadataRequest> fromC =
                std::tr1::static_pointer_cast<MetadataRequest, TransferRequest>(from);
        mCallback(fromC, data);
    }

    inline bool operator==(const MetadataRequest& other) const {
        return mID == other.mID;
    }
    inline bool operator<(const MetadataRequest& other) const {
        return mID < other.mID;
    }

protected:

    const URI mURI;
    std::string mID;
    MetadataCallback mCallback;
    std::tr1::shared_ptr<RemoteFileMetadata> mRemoteFileMetadata;

    MetadataRequest(const URI &uri, PriorityType priority) :
        mURI(uri) {
        mPriority = priority;
        mDeletionRequest = false;

        mID = uri.toString();
    }


    inline void execute_finished(std::tr1::shared_ptr<RemoteFileMetadata> response, ExecuteFinished cb) {
        mRemoteFileMetadata = response;
        cb();
        SILOG(transfer, detailed, "done MetadataRequest execute_finished");
    }

};

typedef std::tr1::shared_ptr<MetadataRequest> MetadataRequestPtr;


/*
 * Handles requests for the data associated with a file:chunk pair
 */
class SIRIKATA_EXPORT ChunkRequest : public MetadataRequest {

public:
    typedef std::tr1::function<void(
            std::tr1::shared_ptr<ChunkRequest> request,
            std::tr1::shared_ptr<const DenseData> response)> ChunkCallback;

	ChunkRequest(const URI &uri, const RemoteFileMetadata &metadata, const Chunk &chunk,
	        PriorityType priority, ChunkCallback cb)
            : MetadataRequest(uri, priority),
            mMetadata(std::tr1::shared_ptr<RemoteFileMetadata>(new RemoteFileMetadata(metadata))),
            mChunk(std::tr1::shared_ptr<Chunk>(new Chunk(chunk))),
            mCallback(cb)
            {
                mDeletionRequest = false;
                mID = chunk.getHash().toString();
            }

	inline const RemoteFileMetadata& getMetadata() {
		return *mMetadata;
	}

	inline const Chunk& getChunk() {
		return *mChunk;
	}

    void execute(std::tr1::shared_ptr<TransferRequest> req, ExecuteFinished cb);

    void execute_finished(std::tr1::shared_ptr<const DenseData> response, ExecuteFinished cb);

    void notifyCaller(std::tr1::shared_ptr<TransferRequest> from);
    void notifyCaller(std::tr1::shared_ptr<TransferRequest> from, DenseDataPtr data);

protected:
    std::tr1::shared_ptr<RemoteFileMetadata> mMetadata;
    std::tr1::shared_ptr<Chunk> mChunk;
    std::tr1::shared_ptr<const DenseData> mDenseData;
    ChunkCallback mCallback;
};

typedef std::tr1::shared_ptr<ChunkRequest> ChunkRequestPtr;

/*
 * Abstract class for providing an algorithm for aggregating priorities from
 * one or more clients for a request
 */
class PriorityAggregationAlgorithm {

public:

    //Return an aggregated priority given the list of priorities
    virtual TransferRequest::PriorityType aggregate(
        const std::map<std::string, TransferRequestPtr> &) const = 0;

    virtual TransferRequest::PriorityType aggregate(
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
    virtual void updatePriority(TransferRequestPtr req, TransferRequest::PriorityType p) = 0;
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
    void setRequestPriority(TransferRequestPtr req, TransferRequest::PriorityType p) {
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
    virtual void updatePriority(TransferRequestPtr req, TransferRequest::PriorityType p) {
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

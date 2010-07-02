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
#include "RemoteFileMetadata.hpp"
#include "TransferHandlers.hpp"
#include "URI.hpp"

namespace Sirikata {
namespace Transfer {

/*
 * Base class for a request going into the transfer pool to set
 * the priority of a request
 */
class TransferRequest {

public:
    typedef std::tr1::function<void()> ExecuteFinished;
    typedef float PriorityType;

	//Return the priority of the request
	inline PriorityType getPriority() const {
		return mPriority;
	}

	//Return a unique identifier for the request
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

	PriorityType mPriority;
	std::string mClientID;

};

/*
 * Handles requests for metadata of a file when all you have is the URI
 */
class MetadataRequest: public TransferRequest {

public:
    typedef std::tr1::function<void(
            std::tr1::shared_ptr<MetadataRequest> request,
            std::tr1::shared_ptr<RemoteFileMetadata> response)> MetadataCallback;

    MetadataRequest(const URI &uri, PriorityType priority, MetadataCallback cb) :
        mURI(uri), mUniqueID(uri.toString()), mCallback(cb) {
        mPriority = priority;
    }

    inline const std::string &getIdentifier() const {
        return mUniqueID;
    }

    inline const URI& getURI() {
        return mURI;
    }

    inline void execute(std::tr1::shared_ptr<TransferRequest> req, ExecuteFinished cb) {
        std::tr1::shared_ptr<MetadataRequest> casted =
                std::tr1::static_pointer_cast<MetadataRequest, TransferRequest>(req);
        HttpNameHandler::getSingleton().resolve(casted, std::tr1::bind(
                &MetadataRequest::execute_finished, this, _1, cb));
    }

    inline void notifyCaller(std::tr1::shared_ptr<TransferRequest> from) {
        std::tr1::shared_ptr<MetadataRequest> fromC =
                std::tr1::static_pointer_cast<MetadataRequest, TransferRequest>(from);
        mCallback(fromC, fromC->mRemoteFileMetadata);
    }

    inline bool operator==(const MetadataRequest& other) const {
        return mUniqueID == other.mUniqueID;
    }
    inline bool operator<(const MetadataRequest& other) const {
        return mUniqueID < other.mUniqueID;
    }

protected:
    const URI mURI;
    const std::string mUniqueID;
    MetadataCallback mCallback;
    std::tr1::shared_ptr<RemoteFileMetadata> mRemoteFileMetadata;

    MetadataRequest(const URI &uri, PriorityType priority) :
        mURI(uri), mUniqueID(uri.toString()) {
        mPriority = priority;
    }

    inline void execute_finished(std::tr1::shared_ptr<RemoteFileMetadata> response, ExecuteFinished cb) {
        mRemoteFileMetadata = response;
        cb();
        SILOG(transfer, debug, "done MetadataRequest execute_finished");
    }

};

/*
 * Handles requests for the data associated with a file:chunk pair
 */
class ChunkRequest : public MetadataRequest {

public:
    typedef std::tr1::function<void(
            std::tr1::shared_ptr<ChunkRequest> request,
            std::tr1::shared_ptr<DenseData> response)> ChunkCallback;

	ChunkRequest(const URI &uri, const RemoteFileMetadata &metadata, const Chunk &chunk,
	        PriorityType priority, ChunkCallback cb)
		: MetadataRequest(uri, priority),
		  mMetadata(std::tr1::shared_ptr<RemoteFileMetadata>(new RemoteFileMetadata(metadata))),
		  mUniqueID(MetadataRequest::mUniqueID + chunk.getHash().convertToHexString()),
		  mChunk(std::tr1::shared_ptr<Chunk>(new Chunk(chunk))),
		  mCallback(cb) {

	}

	inline const RemoteFileMetadata& getMetadata() {
		return *mMetadata;
	}

	inline const Chunk& getChunk() {
		return *mChunk;
	}

	inline void execute(std::tr1::shared_ptr<TransferRequest> req, ExecuteFinished cb) {
        std::tr1::shared_ptr<ChunkRequest> casted =
                std::tr1::static_pointer_cast<ChunkRequest, TransferRequest>(req);
        HttpChunkHandler::getSingleton().get(mMetadata, mChunk, std::tr1::bind(
                &ChunkRequest::execute_finished, this, _1, cb));
	}

	inline void execute_finished(std::tr1::shared_ptr<DenseData> response, ExecuteFinished cb) {
        mDenseData = response;
        cb();
        SILOG(transfer, debug, "done ChunkRequest execute_finished");
	}

    inline void notifyCaller(std::tr1::shared_ptr<TransferRequest> from) {
        std::tr1::shared_ptr<ChunkRequest> fromC =
                std::tr1::static_pointer_cast<ChunkRequest, TransferRequest>(from);
        mCallback(fromC, fromC->mDenseData);
    }

protected:
    std::tr1::shared_ptr<RemoteFileMetadata> mMetadata;
    const std::string mUniqueID;
    std::tr1::shared_ptr<Chunk> mChunk;
    std::tr1::shared_ptr<DenseData> mDenseData;
    ChunkCallback mCallback;

};

/*
 * Abstract class for providing an algorithm for aggregating priorities from
 * one or more clients for a request
 */
class PriorityAggregationAlgorithm {

public:

	//Return an aggregated priority given the list of priorities
	virtual TransferRequest::PriorityType aggregate(
	        std::map<std::string, std::tr1::shared_ptr<TransferRequest> > &) const = 0;

	virtual ~PriorityAggregationAlgorithm() {
	}

};

/*
 * Pools requests for objects to enter the transfer mediator
 */
class TransferPool {

	const std::string mClientID;
	ThreadSafeQueue<std::tr1::shared_ptr<TransferRequest> > mDeltaQueue;

public:
	TransferPool(const std::string &clientID)
		: mClientID(clientID) {

	}

	//Returns client identifier
	inline const std::string& getClientID() const {
		return mClientID;
	}

	//Puts a request into the pool
	inline void addRequest(std::tr1::shared_ptr<TransferRequest> req) {
		if(req != NULL) req->setClientID(mClientID);
		mDeltaQueue.push(req);
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

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
#include "RemoteFileMetadata.hpp"
#include "URI.hpp"

namespace Sirikata {
namespace Transfer {

/*
 * Base class for a request going into the transfer pool to set
 * the priority of a request
 */
class TransferRequest {

public:
	typedef float PriorityType;

	//Return the priority of the request
	inline PriorityType getPriority() const {
		return mPriority;
	}

	//Return a unique identifier for the request
	virtual const std::string &getIdentifier() const = 0;

	virtual ~TransferRequest() {
	}

protected:
	PriorityType mPriority;

};

/*
 * Handles requests for metadata of a file when all you have is the URI
 */
class MetadataRequest : public TransferRequest {

protected:
	const URI mURI;
	const std::string mUniqueID;

public:
	MetadataRequest(const URI &uri, PriorityType priority)
		: mURI(uri), mUniqueID(uri.toString()) {
		mPriority = priority;
	}

	inline const std::string &getIdentifier() const {
		return mUniqueID;
	}

	inline const URI& getURI() {
		return mURI;
	}
};

/*
 * Handles requests for the data associated with a file:chunk pair
 */
class ChunkRequest : public MetadataRequest {

protected:
	const RemoteFileMetadata mMetadata;
	const std::string mUniqueID;
	const Fingerprint mChunkID;

public:
	ChunkRequest(const URI &uri, const RemoteFileMetadata &metadata, const Fingerprint &chunkID, PriorityType priority)
		: MetadataRequest(uri, priority), mMetadata(metadata), mUniqueID(MetadataRequest::mUniqueID + chunkID.convertToHexString()), mChunkID(chunkID) {

	}

	inline const RemoteFileMetadata &getMetadata() {
		return mMetadata;
	}

	inline const Fingerprint &getChunkID() {
		return mChunkID;
	}

};

/*
 * Abstract class for providing an algorithm for aggregating priorities from
 * one or more clients for a request
 */
class PriorityAggregationAlgorithm {

public:

	//Return an aggregated priority given the list of priorities
	virtual TransferRequest::PriorityType aggregate(
			std::tr1::shared_ptr<TransferRequest> req, std::map<std::string, TransferRequest::PriorityType> &) const = 0;

	virtual ~PriorityAggregationAlgorithm() {
	}

};

/*
 * Pools requests for objects to enter the transfer mediator
 */
class TransferPool {

	typedef Task::GenEventManager::EventListener EventListener;

	const std::string mClientID;
	const EventListener &mEventListener;
	ThreadSafeQueue<std::tr1::shared_ptr<TransferRequest> > mDeltaQueue;

public:
	TransferPool(const std::string &clientID, const EventListener &listener)
		: mClientID(clientID), mEventListener(listener) {

	}

	//Returns client identifier
	inline const std::string getClientID() const {
		return mClientID;
	}

	//Returns the listener that should be called when a request is fulfilled
	inline const EventListener getListener() const {
		return mEventListener;
	}

	//Puts a request into the pool
	inline void addRequest(std::tr1::shared_ptr<TransferRequest> req) {
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

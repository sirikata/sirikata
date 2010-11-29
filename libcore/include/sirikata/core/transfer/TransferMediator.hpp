/*  Sirikata Transfer -- Content Transfer mediation
 *  TransferMediator.hpp
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
/*  Created on: Jan 15, 2010 */

#ifndef SIRIKATA_TransferMediator_HPP__
#define SIRIKATA_TransferMediator_HPP__

#include "SimplePriorityAggregation.hpp"

#include <map>
#include <vector>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/lambda/lambda.hpp>
#include <sirikata/core/network/IOService.hpp>
#include <sirikata/core/network/Asio.hpp>
#include <sirikata/core/util/Thread.hpp>
#include <sirikata/core/util/Singleton.hpp>

namespace Sirikata {
namespace Transfer {

using namespace boost::multi_index;

/*
 * Mediates requests for name lookups and chunk downloads
 */
class SIRIKATA_EXPORT TransferMediator
    : public AutoSingleton<TransferMediator> {

    /*
     * Used to aggregate requests from different clients. If multiple clients request
     * the same file, this object keeps track of the original separate requests so that
     * each client's callback can be called when the request finishes. It also uses the
     * PriorityAggregation interface to aggregate multiple client priorities into a single
     * aggregated priority.
     */
	class AggregateRequest {
	public:
	    //Stores the aggregated priority
		TransferRequest::PriorityType mPriority;

	private:
		//Maps each client's string ID to the original TransferRequest object
		std::map<std::string, std::tr1::shared_ptr<TransferRequest> > mTransferReqs;

		//Aggregated request unique identifier
		const std::string mIdentifier;

		//Updates the aggregated priority from each client's priority when needed
		void updateAggregatePriority();

	public:
		//Returns a map from each client ID to their original TransferRequest object
		const std::map<std::string, std::tr1::shared_ptr<TransferRequest> > & getTransferRequests() const;

		//Since there is overlap between requests here, this returns a single TransferRequest from the list of clients
		std::tr1::shared_ptr<TransferRequest> getSingleRequest();

		//Adds an additional client's request
		void setClientPriority(std::tr1::shared_ptr<TransferRequest> req);

        //Removes a client request from this aggregate request
        void removeClient(std::string clientID);

		//Returns a unique identifier for this aggregated request
		const std::string& getIdentifier() const;

		//Returns the aggregated priority value
		TransferRequest::PriorityType getPriority() const;

		//Pass in the first client's request
		AggregateRequest(std::tr1::shared_ptr<TransferRequest> req);
	};

	//lock this to access mAggregatedList
	boost::mutex mAggMutex;

	//tags used to index AggregateList (see boost::multi_index)
	struct tagID{};
	struct tagPriority{};

	/*
	 * This multi_index_container allows the efficient retrieval of an AggregateRequest
	 * either by its identifier or sorted by its priority
	 */
	typedef multi_index_container<
		std::tr1::shared_ptr<AggregateRequest>,
		indexed_by<
			hashed_unique<tag<tagID>, const_mem_fun<AggregateRequest,const std::string &,&AggregateRequest::getIdentifier> >,
			ordered_non_unique<tag<tagPriority>,
			member<AggregateRequest,TransferRequest::PriorityType,&AggregateRequest::mPriority>,
			std::greater<TransferRequest::PriorityType> >
		>
	> AggregateList;
	AggregateList mAggregateList;

	//access iterators for AggregateList for convenience (see boost::multi_index)
	typedef AggregateList::index<tagID>::type AggregateListByID;
	typedef AggregateList::index<tagPriority>::type AggregateListByPriority;

	/*
	 * Used to process the queue of requests coming from a single client.
	 * There is one PoolWorker for each client, and this makes it so that the client's
	 * thread doesn't have to block when inserting a request. Instead, the PoolWorkers
	 * block against each other when inserting the request into the aggregated list.
	 */
	class PoolWorker {
	private:
	    //The TransferPool associated with this worker
		std::tr1::shared_ptr<TransferPool> mTransferPool;

		//The worker's thread
		Thread * mWorkerThread;

		//Set to true when worker should shut down
		bool mCleanup;

		//Runs the worker thread
	    void run();

	public:
		//Initialize with the associated TransferPool
		PoolWorker(std::tr1::shared_ptr<TransferPool> transferPool);

		//Returns the TransferPool this was initialized with
		std::tr1::shared_ptr<TransferPool> getTransferPool() const;

		//Returns the worker's thread
		Thread * getThread() const;

		//Signal to shut down
		void cleanup();
	};

    friend class PoolWorker;
    //Helper for compatibility with compilers where TransferPool declaring
    //TransferMediator as a friend does not give PoolWorker access
    static inline std::tr1::shared_ptr<TransferRequest> getRequest(std::tr1::shared_ptr<TransferPool> pool) {
        return pool->getRequest();
    }

	//Maps a client ID string to the PoolWorker class
	typedef std::map<std::string, std::tr1::shared_ptr<PoolWorker> > PoolType;
	//Stores the list of pools
	PoolType mPools;
	//lock this to access mPools
	boost::shared_mutex mPoolMutex;

	//Set to true to signal shutdown
	bool mCleanup;
	//Number of outstanding requests
	uint32 mNumOutstanding;

	//TransferMediator's worker thread
	Thread* mThread;

    //Main thread that handles the input pools
    void mediatorThread();

    //Callback for when an executed request finishes
    void execute_finished(std::tr1::shared_ptr<TransferRequest> req, std::string id);

    //Check our internal queue to see what request to process next
    void checkQueue();

public:
	static TransferMediator& getSingleton();
	static void destroy();

	TransferMediator();

	/*
	 * Used to register a client that has a pool of requests it needs serviced by the transfer mediator
	 * @param clientID	Should be a string that uniquely identifies the client
	 */
	std::tr1::shared_ptr<TransferPool> registerClient(const std::string clientID);

	//Call when system should be shut down
	void cleanup();
};

}
}

#endif /* SIRIKATA_TransferMediator_HPP__ */

// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_CORE_TRANSFER_AGGREGATED_TRANSFER_POOL_HPP_
#define _SIRIKATA_CORE_TRANSFER_AGGREGATED_TRANSFER_POOL_HPP_

#include <sirikata/core/transfer/TransferPool.hpp>
#include <sirikata/core/transfer/MaxPriorityAggregation.hpp>

namespace Sirikata {
namespace Transfer {

/** AggregatedTransferPool manages multiple requests for the same
 *  resource. It only submits a single request to the
 *  Transfermediator, but aggregates priorities for the requests
 *  according to a user-provided function.  When a resource is
 *  returned by the TransferMediator, it triggers all callbacks for
 *  the requested data.
 */
class AggregatedTransferPool : public TransferPool {
public:
    virtual ~AggregatedTransferPool() {
        boost::unique_lock<boost::mutex> lock(mMutex);
        for(RequestDataMap::iterator it = mRequestData.begin(); it != mRequestData.end(); it++) {
            setRequestDeletion(it->second.aggregateRequest);
            mDeltaQueue.push(it->second.aggregateRequest);
        }
    }

    //Puts a request into the pool
    virtual void addRequest(TransferRequestPtr req) {
        if (!req) {
            mDeltaQueue.push(req);
            return;
        }

        boost::unique_lock<boost::mutex> lock(mMutex);

        RequestDataMap::iterator it = mRequestData.find(req->getIdentifier());

        if (it == mRequestData.end()) {
            // Original addition. Create real request, add it
            RequestData data;
            data.inputRequests.push_back(req);

            // To create the real request and be able to forward callbacks
            //properly, we need to down cast to figure out the type.
            MetadataRequestPtr metadata_req = std::tr1::dynamic_pointer_cast<MetadataRequest>(req);
            if (metadata_req) {
                data.aggregateRequest = TransferRequestPtr(
                    new MetadataRequest(
                        metadata_req->getURI(), metadata_req->getPriority(),
                        std::tr1::bind(&AggregatedTransferPool::handleMetadata, this, req->getIdentifier(), _1, _2)
                    )
                );
            }
            ChunkRequestPtr chunk_req = std::tr1::dynamic_pointer_cast<ChunkRequest>(req);
            if (chunk_req) {
                data.aggregateRequest = TransferRequestPtr(
                    new ChunkRequest(
                        chunk_req->getURI(), chunk_req->getMetadata(), chunk_req->getChunk(), chunk_req->getPriority(),
                        std::tr1::bind(&AggregatedTransferPool::handleChunk, this, req->getIdentifier(), _1, _2)
                    )
                );
            }
            DirectChunkRequestPtr direct_chunk_req = std::tr1::dynamic_pointer_cast<DirectChunkRequest>(req);
            if (direct_chunk_req) {
                data.aggregateRequest = TransferRequestPtr(
                    new DirectChunkRequest(
                        direct_chunk_req->getChunk(), direct_chunk_req->getPriority(),
                        std::tr1::bind(&AggregatedTransferPool::handleDirectChunk, this, req->getIdentifier(), _1, _2)
                    )
                );
            }
            UploadRequestPtr upload_req = std::tr1::dynamic_pointer_cast<UploadRequest>(req);
            if (upload_req) {
                data.aggregateRequest = TransferRequestPtr(
                    new UploadRequest(
                        upload_req->oauth(), upload_req->files(), upload_req->path(), upload_req->params(), upload_req->getPriority(),
                        std::tr1::bind(&AggregatedTransferPool::handleUpload, this, req->getIdentifier(), _1, _2)
                    )
                );
            }

            if (!data.aggregateRequest) {
                SILOG(transfer, error, "Failed to add request to AggregatedTransferPool: unhandled request type.");
                return;
            }

            mRequestData[req->getIdentifier()] = data;
            it = mRequestData.find(req->getIdentifier());
        }
        else {
            // Add to existing aggregate.
            it->second.inputRequests.push_back(req);
        }

        // Whether this is new or not, we can just set its basic properties, and
        // we'll definitely need to recompute the
        setRequestClientID(it->second.aggregateRequest);
        setRequestPriority(it->second.aggregateRequest, mAggregationAlgorithm->aggregate(it->second.inputRequests));

        mDeltaQueue.push(it->second.aggregateRequest);
    }

    //Updates priority of a request in the pool
    virtual void updatePriority(TransferRequestPtr req, Priority p) {
        boost::unique_lock<boost::mutex> lock(mMutex);

        RequestDataMap::iterator it = mRequestData.find(req->getIdentifier());
        // We want
        //assert(it != mRequestData.end());
        // but threading and the fact that callbacks are posted across
        // strands means we might end up cleaning out a request and
        // then very soon after get an updatePriority request, and
        // then the callback is made which would make the request by
        // the client invalid.
        if (it == mRequestData.end()) return;

        // Update priority of individual request
        setRequestPriority(req, p);
        // Update aggregate priority
        setRequestPriority(it->second.aggregateRequest, mAggregationAlgorithm->aggregate(it->second.inputRequests));
        mDeltaQueue.push(it->second.aggregateRequest);
    }

    //Updates priority of a request in the pool
    inline void deleteRequest(TransferRequestPtr req) {
        boost::unique_lock<boost::mutex> lock(mMutex);

        RequestDataMap::iterator it = mRequestData.find(req->getIdentifier());
        // We want
        //assert(it != mRequestData.end());
        // but can't assume it, see note in updatePriority
        if (it == mRequestData.end()) return;

        // Remove from the list of input requests
        for(TransferRequestList::iterator in_it = it->second.inputRequests.begin();
            in_it != it->second.inputRequests.end();
            in_it++) {
            // Strict equality on the shared ptrs, the "unique" ids on
            // TransferRequests are not actually unique
            if (*in_it == req) {
                it->second.inputRequests.erase(in_it);
                break;
            }
        }

        // If we've run out of requests, forward the deletion on via the
        // aggregate and clean up.
        if (it->second.inputRequests.empty()) {
            setRequestDeletion(it->second.aggregateRequest);
            mDeltaQueue.push(it->second.aggregateRequest);
            mRequestData.erase(it);
        }
        else {
            // Otherwise, update priority
            setRequestPriority(it->second.aggregateRequest, mAggregationAlgorithm->aggregate(it->second.inputRequests));
            mDeltaQueue.push(it->second.aggregateRequest);
        }

    }

private:
    // Friend in TransferMediator so it can construct, call getRequest
    friend class TransferMediator;

    AggregatedTransferPool(const std::string &clientID)
     : TransferPool(clientID)
    {
        // FIXME should be customizable
        mAggregationAlgorithm = new MaxPriorityAggregation();
    }

    //Returns an item from the pool. Blocks if pool is empty.
    inline std::tr1::shared_ptr<TransferRequest> getRequest() {
        std::tr1::shared_ptr<TransferRequest> retval;
        mDeltaQueue.blockingPop(retval);
        return retval;
    }


    // Handle metadata callback, sending data to callbacks
    void handleMetadata(const String input_identifier, MetadataRequestPtr req, RemoteFileMetadataPtr response) {
        // Since callbacks may manipulate this TransferPool, grab the data
        // needed for callbacks, unlock, then invoke them
        TransferRequestList inputRequests;
        {
            boost::unique_lock<boost::mutex> lock(mMutex);

            // Need to search by input identifier because the unique id of req is
            // different than the one we index by.
            RequestDataMap::iterator it = mRequestData.find(input_identifier);
            // We'd like to do
            //assert(it != mRequestData.end());
            // but because of threading that could be incorrect -- the
            // TransferMediator could finish and post the callback,
            // then we might have another request for the same data
            // come in, causing us to re-request it. We think its a
            // priority update, but TransferMediator now adds it as an
            // additional request. Instead, we can just ignore the
            // callback if we don't have data for it -- we've already
            // invoked the callback that caused the problem.
            if (it == mRequestData.end()) return;

            inputRequests.swap(it->second.inputRequests);
            mRequestData.erase(it);
        }

        // Just forward the callback and clear out the data. Because the request
        // isn't guaranteed to have the response, we need to
        for(TransferRequestList::iterator in_it = inputRequests.begin();
            in_it != inputRequests.end();
            in_it++) {
            MetadataRequestPtr metadata_req = std::tr1::dynamic_pointer_cast<MetadataRequest>(*in_it);
            metadata_req->notifyCaller(metadata_req, req, response);
        }
    }
    void handleChunk(const String input_identifier, ChunkRequestPtr req, DenseDataPtr response) {
        // Since callbacks may manipulate this TransferPool, grab the data
        // needed for callbacks, unlock, then invoke them
        TransferRequestList inputRequests;
        {
            boost::unique_lock<boost::mutex> lock(mMutex);

            // Need to search by input identifier because the unique id of req is
            // different than the one we index by.
            RequestDataMap::iterator it = mRequestData.find(input_identifier);
            // We'd like to do
            //assert(it != mRequestData.end());
            // but because of threading that could be incorrect -- the
            // TransferMediator could finish and post the callback,
            // then we might have another request for the same data
            // come in, causing us to re-request it. We think its a
            // priority update, but TransferMediator now adds it as an
            // additional request. Instead, we can just ignore the
            // callback if we don't have data for it -- we've already
            // invoked the callback that caused the problem.
            if (it == mRequestData.end()) return;

            inputRequests.swap(it->second.inputRequests);
            mRequestData.erase(it);
        }

        // Just forward the callback and clear out the data. Because the request
        // isn't guaranteed to have the response, we need to
        for(TransferRequestList::iterator in_it = inputRequests.begin();
            in_it != inputRequests.end();
            in_it++) {
            ChunkRequestPtr chunk_req = std::tr1::dynamic_pointer_cast<ChunkRequest>(*in_it);
            chunk_req->notifyCaller(chunk_req, req, response);
        }
    }

    void handleDirectChunk(const String input_identifier, DirectChunkRequestPtr req, DenseDataPtr response) {
        TransferRequestList inputRequests;
        {
            boost::unique_lock<boost::mutex> lock(mMutex);
            RequestDataMap::iterator it = mRequestData.find(input_identifier);
            if (it == mRequestData.end()) return;
            inputRequests.swap(it->second.inputRequests);
            mRequestData.erase(it);
        }

        for(TransferRequestList::iterator in_it = inputRequests.begin();
            in_it != inputRequests.end();
            in_it++) {
            DirectChunkRequestPtr direct_chunk_req = std::tr1::dynamic_pointer_cast<DirectChunkRequest>(*in_it);
            direct_chunk_req->notifyCaller(direct_chunk_req, req, response);
        }
    }

    void handleUpload(const String input_identifier, UploadRequestPtr req, const Transfer::URI& response) {
        TransferRequestList inputRequests;
        {
            boost::unique_lock<boost::mutex> lock(mMutex);
            RequestDataMap::iterator it = mRequestData.find(input_identifier);
            if (it == mRequestData.end()) return;
            inputRequests.swap(it->second.inputRequests);
            mRequestData.erase(it);
        }

        for(TransferRequestList::iterator in_it = inputRequests.begin();
            in_it != inputRequests.end();
            in_it++) {
            UploadRequestPtr upload_req = std::tr1::dynamic_pointer_cast<UploadRequest>(*in_it);
            // notifyCaller copies info into upload_req, so response doesn't
            // need to be passed through
            upload_req->notifyCaller(upload_req, req);
        }
    }

    // Aggregation implementation. FIXME This should be customizable
    PriorityAggregationAlgorithm* mAggregationAlgorithm;

    // For each unique request ID, we need to maintain the original requests and
    // the request
    boost::mutex mMutex; // Protection for mRequestData
    typedef std::vector<TransferRequestPtr> TransferRequestList;
    struct RequestData {
        TransferRequestList inputRequests;
        TransferRequestPtr aggregateRequest;
    };
    // UniqueID -> RequestData
    typedef std::tr1::unordered_map<String, RequestData> RequestDataMap;
    RequestDataMap mRequestData;

    // Requests to the TransferMediator
    ThreadSafeQueue<TransferRequestPtr> mDeltaQueue;
};

} // namespace Transfer
} // namespace Sirikata

#endif //_SIRIKATA_CORE_TRANSFER_AGGREGATED_TRANSFER_POOL_HPP_

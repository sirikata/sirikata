#include <sirikata/core/util/Standard.hh>

#include <sirikata/core/transfer/TransferMediator.hpp>
#include <stdio.h>

using namespace std;

AUTO_SINGLETON_INSTANCE(Sirikata::Transfer::TransferMediator);


using namespace Sirikata;
using namespace Sirikata::Transfer;

namespace Sirikata{
namespace Transfer{

/*
 * TransferMediator definitions
 */

TransferMediator& TransferMediator::getSingleton() {
    return AutoSingleton<TransferMediator>::getSingleton();
}
void TransferMediator::destroy() {
    AutoSingleton<TransferMediator>::destroy();
}

TransferMediator::TransferMediator() {
    mCleanup = false;
    mNumOutstanding = 0;
    mThread = new Thread(std::tr1::bind(&TransferMediator::mediatorThread, this));
}

void TransferMediator::mediatorThread() {
    while(!mCleanup) {
        checkQueue();
        boost::this_thread::sleep(boost::posix_time::milliseconds(500));
    }
    for(PoolType::iterator pool = mPools.begin(); pool != mPools.end(); pool++) {
        pool->second->cleanup();
        pool->second->getTransferPool()->addRequest(std::tr1::shared_ptr<TransferRequest>());
    }
    for(PoolType::iterator pool = mPools.begin(); pool != mPools.end(); pool++) {
        pool->second->getThread()->join();
    }
}

std::tr1::shared_ptr<TransferPool> TransferMediator::registerClient(const std::string clientID) {
    std::tr1::shared_ptr<TransferPool> ret(new TransferPool(clientID));

    //Lock exclusive to access map
    boost::upgrade_lock<boost::shared_mutex> lock(mPoolMutex);
    boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);

    //ensure client id doesnt already exist, they should be unique
    PoolType::iterator findClientId = mPools.find(clientID);
    assert(findClientId == mPools.end());

    std::tr1::shared_ptr<PoolWorker> worker(new PoolWorker(ret));
    mPools.insert(PoolType::value_type(clientID, worker));

    return ret;
}

void TransferMediator::cleanup() {
    mCleanup = true;
    mThread->join();
}

void TransferMediator::execute_finished(std::tr1::shared_ptr<TransferRequest> req, std::string id) {
    boost::unique_lock<boost::mutex> lock(mAggMutex, boost::defer_lock_t());
    lock.lock();

    AggregateListByID& idIndex = mAggregateList.get<tagID>();
    AggregateListByID::iterator findID = idIndex.find(id);
    if(findID == idIndex.end()) {
        //This can happen now if a request was canceled but it was already outstanding
        mNumOutstanding--;
        lock.unlock();
        checkQueue();
        return;
    }

    const std::map<std::string, std::tr1::shared_ptr<TransferRequest> >&
        allReqs = (*findID)->getTransferRequests();

    for(std::map<std::string, std::tr1::shared_ptr<TransferRequest> >::const_iterator
            it = allReqs.begin(); it != allReqs.end(); it++) {
        SILOG(transfer, debug, "Notifying a caller that TransferRequest is complete");
        it->second->notifyCaller(req);
    }

    mAggregateList.erase(findID);

    mNumOutstanding--;
    lock.unlock();
    SILOG(transfer, debug, "done transfer mediator execute_finished");
    checkQueue();
}

void TransferMediator::checkQueue() {
    boost::unique_lock<boost::mutex> lock(mAggMutex, boost::defer_lock_t());

    lock.lock();

    AggregateListByPriority & priorityIndex = mAggregateList.get<tagPriority>();
    AggregateListByPriority::iterator findTop = priorityIndex.begin();

    if(findTop != priorityIndex.end()) {
        std::string topId = (*findTop)->getIdentifier();

        SILOG(transfer, debug, priorityIndex.size() << " length agg list, top priority "
                << (*findTop)->getPriority() << " id " << topId);

        std::tr1::shared_ptr<TransferRequest> req = (*findTop)->getSingleRequest();

        if(mNumOutstanding == 0) {
            mNumOutstanding++;
            req->execute(req, std::tr1::bind(&TransferMediator::execute_finished, this, req, topId));
        }

    } else {
        //SILOG(transfer, debug, priorityIndex.size() << " length agg list");
    }

    lock.unlock();
}


/*
 * TransferMediator::AggregateRequest definitions
 */

void TransferMediator::AggregateRequest::updateAggregatePriority() {
    TransferRequest::PriorityType newPriority = SimplePriorityAggregation::aggregate(mTransferReqs);
    mPriority = newPriority;
}

const std::map<std::string, std::tr1::shared_ptr<TransferRequest> >& TransferMediator::AggregateRequest::getTransferRequests() const {
    return mTransferReqs;
}

std::tr1::shared_ptr<TransferRequest> TransferMediator::AggregateRequest::getSingleRequest() {
    std::map<std::string, std::tr1::shared_ptr<TransferRequest> >::iterator it = mTransferReqs.begin();
    return it->second;
}

void TransferMediator::AggregateRequest::setClientPriority(std::tr1::shared_ptr<TransferRequest> req) {
    const std::string& clientID = req->getClientID();
    std::map<std::string, std::tr1::shared_ptr<TransferRequest> >::iterator findClient = mTransferReqs.find(clientID);
    if(findClient == mTransferReqs.end()) {
        mTransferReqs[clientID] = req;
        updateAggregatePriority();
    } else if(findClient->second->getPriority() != req->getPriority()) {
        findClient->second = req;
        updateAggregatePriority();
    } else {
        findClient->second = req;
    }
}

void TransferMediator::AggregateRequest::removeClient(std::string clientID) {
    std::map<std::string, std::tr1::shared_ptr<TransferRequest> >::iterator findClient = mTransferReqs.find(clientID);
    if(findClient != mTransferReqs.end()) {
        mTransferReqs.erase(findClient);
    }
}

const std::string& TransferMediator::AggregateRequest::getIdentifier() const {
    return mIdentifier;
}

TransferRequest::PriorityType TransferMediator::AggregateRequest::getPriority() const {
    return mPriority;
}

TransferMediator::AggregateRequest::AggregateRequest(std::tr1::shared_ptr<TransferRequest> req)
    : mIdentifier(req->getIdentifier()) {
    setClientPriority(req);
}

/*
 * TransferMediator::PoolWorker definitions
 */

TransferMediator::PoolWorker::PoolWorker(std::tr1::shared_ptr<TransferPool> transferPool)
    : mTransferPool(transferPool), mCleanup(false) {
    mWorkerThread = new Thread(std::tr1::bind(&PoolWorker::run, this));
}

std::tr1::shared_ptr<TransferPool> TransferMediator::PoolWorker::getTransferPool() const {
    return mTransferPool;
}

Thread * TransferMediator::PoolWorker::getThread() const {
    return mWorkerThread;
}

void TransferMediator::PoolWorker::cleanup() {
    mCleanup = true;
}

void TransferMediator::PoolWorker::run() {
    while(!mCleanup) {
        std::tr1::shared_ptr<TransferRequest> req = TransferMediator::getRequest(mTransferPool);
        if(req == NULL) {
            continue;
        }
        //SILOG(transfer, debug, "worker got one!");

        boost::unique_lock<boost::mutex> lock(TransferMediator::getSingleton().mAggMutex);
        AggregateListByID& idIndex = TransferMediator::getSingleton().mAggregateList.get<tagID>();
        AggregateListByID::iterator findID = idIndex.find(req->getIdentifier());

        //Check if this request already exists
        if(findID != idIndex.end()) {
            //Check if this request is for deleting
            if(req->isDeletionRequest()) {
                const std::map<std::string, std::tr1::shared_ptr<TransferRequest> >&
                    allReqs = (*findID)->getTransferRequests();

                std::map<std::string,
                    std::tr1::shared_ptr<TransferRequest> >::const_iterator findClient =
                    allReqs.find(req->getClientID());

                /* If the client isn't in the aggregated request, it must have already
                 * been deleted, or the deletion request is invalid
                 */
                if(findClient == allReqs.end()) {
                    continue;
                }

                if(allReqs.size() > 1) {
                    /* If there are more than one, we need to just delete the single client
                     * from the aggregate request
                     */
                    (*findID)->removeClient(req->getClientID());
                } else {
                    // If only one in the list, we can erase the entire request
                    TransferMediator::getSingleton().mAggregateList.erase(findID);
                }
            } else {
                //store original aggregated priority for later
                TransferRequest::PriorityType oldAggPriority = (*findID)->getPriority();

                //Update the priority of this client
                (*findID)->setClientPriority(req);

                //And check if it's changed, we need to update the index
                TransferRequest::PriorityType newAggPriority = (*findID)->getPriority();
                if(oldAggPriority != newAggPriority) {
                    using boost::lambda::_1;
                    //Convert the iterator to the priority one and update
                    AggregateListByPriority::iterator byPriority =
                            TransferMediator::getSingleton().mAggregateList.project<tagPriority>(findID);
                    AggregateListByPriority & priorityIndex =
                            TransferMediator::getSingleton().mAggregateList.get<tagPriority>();
                    priorityIndex.modify_key(byPriority, _1=newAggPriority);
                }
            }
        } else {
            //Make a new one and insert it
            //SILOG(transfer, debug, "worker id " << mTransferPool->getClientID() << " adding url " << req->getIdentifier());
            std::tr1::shared_ptr<AggregateRequest> newAggReq(new AggregateRequest(req));
            TransferMediator::getSingleton().mAggregateList.insert(newAggReq);
        }

    }
}

}
}

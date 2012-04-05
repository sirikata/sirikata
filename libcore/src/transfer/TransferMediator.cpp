#include <sirikata/core/util/Standard.hh>

#include <sirikata/core/transfer/TransferMediator.hpp>
#include <sirikata/core/transfer/MaxPriorityAggregation.hpp>
#include <sirikata/core/util/Timer.hpp>
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
    mAggregationAlgorithm = new MaxPriorityAggregation();
    mThread = new Thread("TransferMediator", std::tr1::bind(&TransferMediator::mediatorThread, this));
}

TransferMediator::~TransferMediator() {
    delete mAggregationAlgorithm;
}

void TransferMediator::mediatorThread() {
    while(!mCleanup) {
        checkQueue();

        /**
           Ewen and I weren't able to identify why the below call caused a
           deadlock on the shared space hosted on sns30 (could not reproduce on
           other machines).  With the below line, in gdb, with bulletphysics on,
           and servermap-options set to port 6880, we'd get 4 calls to
           checkQueue, followed by no follow up calls.  Not sure what to say.
         */
        //boost::this_thread::sleep(boost::posix_time::milliseconds(5));
        Timer::sleep(Duration::milliseconds(5));
    }
    for(PoolType::iterator pool = mPools.begin(); pool != mPools.end(); pool++) {
        pool->second->cleanup();
        pool->second->getTransferPool()->addRequest(std::tr1::shared_ptr<TransferRequest>());
    }
    for(PoolType::iterator pool = mPools.begin(); pool != mPools.end(); pool++) {
        pool->second->getThread()->join();
    }
}

void TransferMediator::registerPool(TransferPoolPtr pool) {
    //Lock exclusive to access map
    boost::upgrade_lock<boost::shared_mutex> lock(mPoolMutex);
    boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);

    //ensure client id doesnt already exist, they should be unique
    PoolType::iterator findClientId = mPools.find(pool->getClientID());
    assert(findClientId == mPools.end());

    std::tr1::shared_ptr<PoolWorker> worker(new PoolWorker(pool));
    mPools.insert(PoolType::value_type(pool->getClientID(), worker));
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
        SILOG(transfer, detailed, "Notifying a caller that TransferRequest is complete");
        it->second->notifyCaller(it->second, req);
    }

    mAggregateList.erase(findID);

    mNumOutstanding--;
    lock.unlock();
    SILOG(transfer, detailed, "done transfer mediator execute_finished");
    checkQueue();
}

void TransferMediator::checkQueue() {
    boost::unique_lock<boost::mutex> lock(mAggMutex, boost::defer_lock_t());

    lock.lock();

    AggregateListByPriority & priorityIndex = mAggregateList.get<tagPriority>();
    AggregateListByPriority::iterator findTop = priorityIndex.begin();

    if(findTop != priorityIndex.end()) {
        std::string topId = (*findTop)->getIdentifier();
        SILOG(transfer, detailed, priorityIndex.size() << " length agg list, top priority "
                << (*findTop)->getPriority() << " id " << topId);
    }

    // While we have free slots and there are items left, scan for items that
    // haven't been started yet that we can process.
    while(findTop != priorityIndex.end() && mNumOutstanding < 10) {
        if (!(*findTop)->mExecuting) {
            mNumOutstanding++;
            (*findTop)->mExecuting = true;
            std::tr1::shared_ptr<TransferRequest> req = (*findTop)->getSingleRequest();
            req->execute(
                req,
                std::tr1::bind(&TransferMediator::execute_finished, this,
                    req, (*findTop)->getIdentifier())
            );
        }
        findTop++;
    }

    lock.unlock();
}


/*
 * TransferMediator::AggregateRequest definitions
 */

void TransferMediator::AggregateRequest::updateAggregatePriority() {
    Priority newPriority = TransferMediator::getSingleton().mAggregationAlgorithm->aggregate(mTransferReqs);
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

Priority TransferMediator::AggregateRequest::getPriority() const {
    return mPriority;
}

TransferMediator::AggregateRequest::AggregateRequest(std::tr1::shared_ptr<TransferRequest> req)
 : mIdentifier(req->getIdentifier()),
   mExecuting(false)
{
    setClientPriority(req);
}

/*
 * TransferMediator::PoolWorker definitions
 */

TransferMediator::PoolWorker::PoolWorker(std::tr1::shared_ptr<TransferPool> transferPool)
    : mTransferPool(transferPool), mCleanup(false) {
    mWorkerThread = new Thread("TransferMediator Worker", std::tr1::bind(&PoolWorker::run, this));
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
                Priority oldAggPriority = (*findID)->getPriority();

                //Update the priority of this client
                (*findID)->setClientPriority(req);

                //And check if it's changed, we need to update the index
                Priority newAggPriority = (*findID)->getPriority();
                if(oldAggPriority != newAggPriority) {
                    //Convert the iterator to the priority one and update
                    AggregateListByPriority::iterator byPriority =
                            TransferMediator::getSingleton().mAggregateList.project<tagPriority>(findID);
                    AggregateListByPriority & priorityIndex =
                            TransferMediator::getSingleton().mAggregateList.get<tagPriority>();
                    priorityIndex.modify_key(byPriority, boost::lambda::_1=newAggPriority);
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

void TransferMediator::registerContext(Context* ctx) {
    if (ctx->commander()) {
        ctx->commander()->registerCommand(
            "transfer.mediator.requests.list",
            std::tr1::bind(&TransferMediator::commandListRequests, this, _1, _2, _3)
        );
    }
}

void TransferMediator::commandListRequests(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid) {
    Command::Result result = Command::EmptyResult();
    result.put( String("requests"), Command::Array());
    Command::Array& requests_ary = result.getArray("requests");

    boost::unique_lock<boost::mutex> lock(mAggMutex);
    AggregateListByPriority& priorityIndex = mAggregateList.get<tagPriority>();
    for(AggregateListByPriority::iterator req_it = priorityIndex.begin(); req_it != priorityIndex.end(); req_it++) {
        requests_ary.push_back(Command::Object());
        requests_ary.back().put("id", (*req_it)->getIdentifier());
        requests_ary.back().put("priority", (*req_it)->getPriority());
    }

    cmdr->result(cmdid, result);
}

}
}

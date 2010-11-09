/*  Sirikata Transfer
 *  DiskManager.cpp
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

#include <sirikata/core/transfer/DiskManager.hpp>

AUTO_SINGLETON_INSTANCE(Sirikata::Transfer::DiskManager);

namespace Sirikata {
namespace Transfer {

DiskManager& DiskManager::getSingleton() {
    return AutoSingleton<DiskManager>::getSingleton();
}
void DiskManager::destroy() {
    AutoSingleton<DiskManager>::destroy();
}

DiskManager::ScanRequest::ScanRequest(Filesystem::Path path, DiskManager::ScanRequest::ScanRequestCallback cb)
    : mCb(cb), mPath(path) {
}

void DiskManager::ScanRequest::execute() {
    std::tr1::shared_ptr<DirectoryListing> badResult;
    std::tr1::shared_ptr<DirectoryListing> dirListing(new DirectoryListing());

    if(!fs::exists(mPath)) {
        mCb(badResult);
    }

    for(fs::directory_iterator it(mPath); it != fs::directory_iterator(); it++) {
        dirListing->push_back(it->path());
    }

    mCb(dirListing);
}

DiskManager::DiskManager() {
    mWorkerThread = new Thread(std::tr1::bind(&DiskManager::workerThread, this));
}

DiskManager::~DiskManager() {
    //Add an empty request to the queue and then wait for workerThread to finish
    std::tr1::shared_ptr<DiskRequest> nullReq;
    boost::unique_lock<boost::mutex> sleep_cv(destroyLock);
    mRequestQueue.push(nullReq);
    destroyCV.wait(sleep_cv);

    delete mWorkerThread;
}

void DiskManager::addRequest(std::tr1::shared_ptr<DiskRequest> req) {
    mRequestQueue.push(req);
}

void DiskManager::workerThread() {
    while(true) {
        std::tr1::shared_ptr<DiskRequest> req;
        mRequestQueue.blockingPop(req);

        if(!req) break;

        req->execute();
    }

    {
        boost::unique_lock<boost::mutex> wake_cv(destroyLock);
        destroyCV.notify_one();
    }
}

}
}

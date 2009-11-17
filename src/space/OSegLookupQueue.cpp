/*  cbr
 *  OSegLookupQueue.cpp
 *
 *  Copyright (c) 2009, Ewen Cheslack-Postava
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
 *  * Neither the name of cbr nor the names of its contributors may
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

#include "OSegLookupQueue.hpp"
#include "ObjectSegmentation.hpp"

namespace CBR {

// OSegLookupList Implementation

size_t OSegLookupQueue::OSegLookupList::ByteSize() const{
    return mTotalSize;
}

size_t OSegLookupQueue::OSegLookupList::size()const {
    return OSegLookupVector::size();
}

OSegLookupQueue::OSegLookup& OSegLookupQueue::OSegLookupList::operator[](size_t where) {
    return OSegLookupVector::operator[](where);
}

void OSegLookupQueue::OSegLookupList::push_back(const OSegLookup& lu) {
    mTotalSize += lu.msg->ByteSize();
    OSegLookupVector::push_back(lu);
}

OSegLookupQueue::OSegLookupList::iterator OSegLookupQueue::OSegLookupList::begin(){
    return OSegLookupVector::begin();
}

OSegLookupQueue::OSegLookupList::iterator OSegLookupQueue::OSegLookupList::end(){
    return OSegLookupVector::end();
}


// OSegLookupQueue Implementation

OSegLookupQueue::OSegLookupQueue(IOStrand* net_strand, ObjectSegmentation* oseg, const PushPredicate& pred)
 : mNetworkStrand(net_strand),
   mOSeg(oseg),
   mPredicate(pred),
   mTotalSize(0)
{
    mOSeg->setListener(this);
}

bool OSegLookupQueue::lookup(CBR::Protocol::Object::ObjectMessage* msg, const LookupCallback& cb) {
    UUID dest_obj = msg->dest_object();

    // First, initiate a lookup in case we have a cached value
    // FIXME we need some way to control whether this actually causes a real lookup since we may only
    // want to check the cache if the predicate fails
    ServerID destServer = mOSeg->lookup(msg->dest_object());

    // If we already have a server, handle the callback right away
    if (destServer != NullServerID) {
        cb(msg, destServer, ResolvedFromCache);
        return true;
    }

    // Otherwise, check if we'll accept the burden of this very heavy packet
    size_t cursize = msg->ByteSize();
    if (!mPredicate(dest_obj, cursize, mTotalSize))
        return false;

    // And if we do, stick it on a list and wait
    mTotalSize += cursize;
    OSegLookup lu;
    lu.msg = msg;
    lu.cb = cb;
    mLookups[dest_obj].push_back(lu);
    return true;
}

void OSegLookupQueue::osegLookupCompleted(const UUID& id, const ServerID& dest) {
    mNetworkStrand->post(
        std::tr1::bind(&OSegLookupQueue::handleLookupCompleted, this, id, dest)
    );
}

void OSegLookupQueue::handleLookupCompleted(const UUID& id, const ServerID& dest) {
    //Now sending messages that we had saved up from oseg lookup calls.
    LookupMap::iterator iterQueueMap = mLookups.find(id);
    if (iterQueueMap == mLookups.end())
        return;

    for (int s=0; s < (signed) ((iterQueueMap->second).size()); ++ s) {
        const OSegLookup& lu = (iterQueueMap->second[s]);
        mTotalSize -= lu.msg->ByteSize();
        lu.cb(lu.msg, dest, ResolvedFromServer);
    }
    mLookups.erase(iterQueueMap);
}

} // namespace CBR

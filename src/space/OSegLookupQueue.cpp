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
#include "Options.hpp"

namespace Sirikata {

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
    mTotalSize += lu.size;
    OSegLookupVector::push_back(lu);
}

OSegLookupQueue::OSegLookupList::iterator OSegLookupQueue::OSegLookupList::begin(){
    return OSegLookupVector::begin();
}

OSegLookupQueue::OSegLookupList::iterator OSegLookupQueue::OSegLookupList::end(){
    return OSegLookupVector::end();
}


// OSegLookupQueue Implementation


OSegLookupQueue::OSegLookupQueue(IOStrand* net_strand, ObjectSegmentation* oseg)
 : mNetworkStrand(net_strand),
   mOSeg(oseg),
   mTotalSize(0)
{
    mMaxLookups = GetOption(OSEG_LOOKUP_QUEUE_SIZE)->as<uint32>();
    mOSeg->setLookupListener(this);
}

CraqEntry OSegLookupQueue::cacheLookup(const UUID& destid) const {
    //if get a cache hit from oseg, do not return;
    return mOSeg->cacheLookup(destid);
}

bool OSegLookupQueue::lookup(Sirikata::Protocol::Object::ObjectMessage* msg, const LookupCallback& cb)
{
  UUID dest_obj = msg->dest_object();
  size_t cursize = msg->ByteSize();

  //if already looking up, do not call lookup on mOSeg;
  LookupMap::const_iterator it = mLookups.find(dest_obj);
  if (it != mLookups.end())
  {
    //we are already looking up the object.  Just add it to mLookups
    // And if we do, stick it on a list and wait
    mTotalSize += cursize;
    OSegLookup lu;
    lu.msg = msg;
    lu.cb = cb;
    lu.size = cursize;
    mLookups[dest_obj].push_back(lu);
    return true;
  }

  //if get a cache hit from oseg, do not return;
  CraqEntry destServer= mOSeg->cacheLookup(dest_obj);
  if (destServer.notNull())
  {
    cb(msg, destServer, ResolvedFromCache);
    return true;
  }

  //if did not get a cache hit, check if have enough room to add it;
  if (mLookups.size() > mMaxLookups)
    return false;

  //  otherwise, do full oseg lookup;
  destServer = mOSeg->lookup(dest_obj);
  // If we already have a server, handle the callback right away
  if (destServer.notNull()) {
    cb(msg, destServer, ResolvedFromCache);
    return true;
  }

  // And if we do, stick it on a list and wait
  mTotalSize += cursize;
  OSegLookup lu;
  lu.msg = msg;
  lu.cb = cb;
  lu.size = cursize;
  mLookups[dest_obj].push_back(lu);
  return true;
}

void OSegLookupQueue::osegLookupCompleted(const UUID& id, const CraqEntry& dest) {
    mNetworkStrand->post(
        std::tr1::bind(&OSegLookupQueue::handleLookupCompleted, this, id, dest)
    );
}

void OSegLookupQueue::handleLookupCompleted(const UUID& id, const CraqEntry& dest) {
    //Now sending messages that we had saved up from oseg lookup calls.
    LookupMap::iterator iterQueueMap = mLookups.find(id);
    if (iterQueueMap == mLookups.end())
        return;

    for (int s=0; s < (signed) ((iterQueueMap->second).size()); ++ s) {
        const OSegLookup& lu = (iterQueueMap->second[s]);
        mTotalSize -= lu.size;
        lu.cb(lu.msg, dest, ResolvedFromServer);
    }
    mLookups.erase(iterQueueMap);
}

} // namespace Sirikata

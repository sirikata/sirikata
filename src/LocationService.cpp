/*  cbr
 *  LocationService.cpp
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

#include "LocationService.hpp"
#include "AlwaysLocationUpdatePolicy.hpp"

namespace CBR {

LocationServiceListener::~LocationServiceListener() {
}

LocationUpdatePolicy::LocationUpdatePolicy(LocationService* locservice)
 : mLocService(locservice)
{
    mLocService->addListener(this);
}

LocationUpdatePolicy::~LocationUpdatePolicy() {
}


LocationService::LocationService(SpaceContext* ctx)
 : PollingService(ctx->mainStrand),
   mContext(ctx)
{
    mProfiler = mContext->profiler->addStage("Location Service");

    mUpdatePolicy = new AlwaysLocationUpdatePolicy(this);

    mContext->serverDispatcher()->registerMessageRecipient(SERVER_PORT_LOCATION, this);
    mContext->objectDispatcher()->registerObjectMessageRecipient(OBJECT_PORT_LOCATION, this);
}

LocationService::~LocationService() {
    delete mUpdatePolicy;

    mContext->serverDispatcher()->unregisterMessageRecipient(SERVER_PORT_LOCATION, this);
    mContext->objectDispatcher()->unregisterObjectMessageRecipient(OBJECT_PORT_LOCATION, this);
}

void LocationService::poll() {
    mProfiler->started();
    service();
    mProfiler->finished();
}

void LocationService::addListener(LocationServiceListener* listener) {
    assert(mListeners.find(listener) == mListeners.end());
    mListeners.insert(listener);
}

void LocationService::removeListener(LocationServiceListener* listener) {
    ListenerList::iterator it = mListeners.find(listener);
    assert(it != mListeners.end());
    mListeners.erase(it);
}

void LocationService::subscribe(ServerID remote, const UUID& uuid) {
    mUpdatePolicy->subscribe(remote, uuid);
}

void LocationService::unsubscribe(ServerID remote, const UUID& uuid) {
    mUpdatePolicy->unsubscribe(remote, uuid);
}

void LocationService::unsubscribe(ServerID remote) {
    mUpdatePolicy->unsubscribe(remote);
}


void LocationService::subscribe(const UUID& remote, const UUID& uuid) {
    mUpdatePolicy->subscribe(remote, uuid);
}

void LocationService::unsubscribe(const UUID& remote, const UUID& uuid) {
    mUpdatePolicy->unsubscribe(remote, uuid);
}

void LocationService::unsubscribe(const UUID& remote) {
    mUpdatePolicy->unsubscribe(remote);
}

void LocationService::notifyLocalObjectAdded(const UUID& uuid, const TimedMotionVector3f& loc, const BoundingSphere3f& bounds) const {
    for(ListenerList::const_iterator it = mListeners.begin(); it != mListeners.end(); it++)
        (*it)->localObjectAdded(uuid, loc, bounds);
}

void LocationService::notifyLocalObjectRemoved(const UUID& uuid) const {
    for(ListenerList::const_iterator it = mListeners.begin(); it != mListeners.end(); it++)
        (*it)->localObjectRemoved(uuid);
}

void LocationService::notifyLocalLocationUpdated(const UUID& uuid, const TimedMotionVector3f& newval) const {
    for(ListenerList::const_iterator it = mListeners.begin(); it != mListeners.end(); it++)
        (*it)->localLocationUpdated(uuid, newval);
}

void LocationService::notifyLocalBoundsUpdated(const UUID& uuid, const BoundingSphere3f& newval) const {
    for(ListenerList::const_iterator it = mListeners.begin(); it != mListeners.end(); it++)
        (*it)->localBoundsUpdated(uuid, newval);
}


void LocationService::notifyReplicaObjectAdded(const UUID& uuid, const TimedMotionVector3f& loc, const BoundingSphere3f& bounds) const {
    for(ListenerList::const_iterator it = mListeners.begin(); it != mListeners.end(); it++)
        (*it)->replicaObjectAdded(uuid, loc, bounds);
}

void LocationService::notifyReplicaObjectRemoved(const UUID& uuid) const {
    for(ListenerList::const_iterator it = mListeners.begin(); it != mListeners.end(); it++)
        (*it)->replicaObjectRemoved(uuid);
}

void LocationService::notifyReplicaLocationUpdated(const UUID& uuid, const TimedMotionVector3f& newval) const {
    for(ListenerList::const_iterator it = mListeners.begin(); it != mListeners.end(); it++)
        (*it)->replicaLocationUpdated(uuid, newval);
}

void LocationService::notifyReplicaBoundsUpdated(const UUID& uuid, const BoundingSphere3f& newval) const {
    for(ListenerList::const_iterator it = mListeners.begin(); it != mListeners.end(); it++)
        (*it)->replicaBoundsUpdated(uuid, newval);
}

} // namespace CBR

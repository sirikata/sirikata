// Copyright (c) 2012 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_LIBPINTO_REPLICATED_LOCATION_UPDATE_LISTENER_HPP_
#define _SIRIKATA_LIBPINTO_REPLICATED_LOCATION_UPDATE_LISTENER_HPP_

#include <sirikata/pintoloc/Platform.hpp>
#include <sirikata/core/util/ListenerProvider.hpp>

namespace Sirikata {

class ReplicatedLocationServiceCache;

/** Listener interface for ReplicatedLocationServiceCache. This is similar to the
 *  Prox::LocationUpdateListener interface, but differs in two ways. First, it
 *  handles all properties instead of just the few used by the query
 *  processor. Second, it doesn't bother forwarding any values because they are
 *  available in the LocCache and we aren't worried about using newer values
 *  (since they could get overwritten while we're waiting for the update to be
 *  processed after being posted across threads).
 */
class SIRIKATA_LIBPINTOLOC_EXPORT ReplicatedLocationUpdateListener {
public:
    virtual ~ReplicatedLocationUpdateListener() {}

    virtual void onObjectAdded(ReplicatedLocationServiceCache* loccache, const ObjectReference& obj) = 0;
    virtual void onObjectRemoved(ReplicatedLocationServiceCache* loccache, const ObjectReference& obj) = 0;
    virtual void onParentUpdated(ReplicatedLocationServiceCache* loccache, const ObjectReference& obj) = 0;
    virtual void onEpochUpdated(ReplicatedLocationServiceCache* loccache, const ObjectReference& obj) = 0;
    virtual void onLocationUpdated(ReplicatedLocationServiceCache* loccache, const ObjectReference& obj) = 0;
    virtual void onOrientationUpdated(ReplicatedLocationServiceCache* loccache, const ObjectReference& obj) = 0;
    virtual void onBoundsUpdated(ReplicatedLocationServiceCache* loccache, const ObjectReference& obj) = 0;
    virtual void onMeshUpdated(ReplicatedLocationServiceCache* loccache, const ObjectReference& obj) = 0;
    virtual void onPhysicsUpdated(ReplicatedLocationServiceCache* loccache, const ObjectReference& obj) = 0;
};

typedef Provider<ReplicatedLocationUpdateListener*> ReplicatedLocationUpdateProvider;

} // namespace Sirikata

#endif //_SIRIKATA_LIBPINTO_REPLICATED_LOCATION_UPDATE_LISTENER_HPP_

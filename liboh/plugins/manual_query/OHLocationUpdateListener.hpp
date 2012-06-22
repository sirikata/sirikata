// Copyright (c) 2012 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_OH_OH_LOCATION_UPDATE_LISTENER_HPP_
#define _SIRIKATA_OH_OH_LOCATION_UPDATE_LISTENER_HPP_

#include <sirikata/oh/Platform.hpp>
#include <sirikata/core/util/ListenerProvider.hpp>

namespace Sirikata {

class OHLocationServiceCache;

/** Listener interface for OHLocationServiceCache. This is similar to the
 *  Prox::LocationUpdateListener interface, but differs in two ways. First, it
 *  handles all properties instead of just the few used by the query
 *  processor. Second, it doesn't bother forwarding any values because they are
 *  available in the LocCache and we aren't worried about using newer values
 *  (since they could get overwritten while we're waiting for the update to be
 *  processed after being posted across threads).
 */
class OHLocationUpdateListener {
public:
    virtual ~OHLocationUpdateListener() {}

    virtual void onObjectAdded(OHLocationServiceCache* loccache, const ObjectReference& obj) = 0;
    virtual void onObjectRemoved(OHLocationServiceCache* loccache, const ObjectReference& obj) = 0;
    virtual void onParentUpdated(OHLocationServiceCache* loccache, const ObjectReference& obj) = 0;
    virtual void onEpochUpdated(OHLocationServiceCache* loccache, const ObjectReference& obj) = 0;
    virtual void onLocationUpdated(OHLocationServiceCache* loccache, const ObjectReference& obj) = 0;
    virtual void onOrientationUpdated(OHLocationServiceCache* loccache, const ObjectReference& obj) = 0;
    virtual void onBoundsUpdated(OHLocationServiceCache* loccache, const ObjectReference& obj) = 0;
    virtual void onMeshUpdated(OHLocationServiceCache* loccache, const ObjectReference& obj) = 0;
    virtual void onPhysicsUpdated(OHLocationServiceCache* loccache, const ObjectReference& obj) = 0;
};

typedef Provider<OHLocationUpdateListener*> OHLocationUpdateProvider;

} // namespace Sirikata

#endif //_SIRIKATA_OH_OH_LOCATION_UPDATE_LISTENER_HPP_

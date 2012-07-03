// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_LIBPINTO_EXTENDED_LOCATION_SERVICE_CACHE_HPP_
#define _SIRIKATA_LIBPINTO_EXTENDED_LOCATION_SERVICE_CACHE_HPP_

#include <sirikata/pintoloc/Platform.hpp>
#include <sirikata/pintoloc/ProxSimulationTraits.hpp>
#include <prox/base/LocationServiceCache.hpp>
#include <sirikata/core/util/MotionVector.hpp>
#include <sirikata/core/util/MotionQuaternion.hpp>
#include <sirikata/core/transfer/URI.hpp>
#include <sirikata/core/util/AggregateBoundingInfo.hpp>

namespace Sirikata {

class ExtendedLocationServiceCache;
typedef std::tr1::shared_ptr<ExtendedLocationServiceCache> ExtendedLocationServiceCachePtr;

/** ExtendedLocationServiceCache defines some additional methods that provide a
 *  more useful interface than the minimum required by libprox's
 *  LocationServiceCache interface, e.g. letting you look up properties by
 *  object ID and check whether an object is currently being tracked.
 */
class SIRIKATA_LIBPINTOLOC_EXPORT ExtendedLocationServiceCache :
        public Prox::LocationServiceCache<ObjectProxSimulationTraits>
{
public:
    virtual ~ExtendedLocationServiceCache() {}

    virtual bool tracking(const ObjectID& id) = 0;

    // We also provide accessors by ID for Proximity generate results.
    // Note: as with the LocationServiceCache interface, these return values to
    // allow for thread-safety.
    virtual TimedMotionVector3f location(const ObjectID& id) = 0;
    virtual TimedMotionQuaternion orientation(const ObjectID& id) = 0;
    virtual AggregateBoundingInfo bounds(const ObjectID& id) = 0;
    virtual Transfer::URI mesh(const ObjectID& id) = 0;
    virtual String physics(const ObjectID& id) = 0;
};

} // namespace Sirikata

#endif //_SIRIKATA_LIBPINTO_EXTENDED_LOCATION_SERVICE_CACHE_HPP_

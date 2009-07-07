/*  cbr
 *  LocationService.hpp
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

#ifndef _CBR_LOCATION_SERVICE_HPP_
#define _CBR_LOCATION_SERVICE_HPP_

#include "Utility.hpp"
#include "MotionVector.hpp"

namespace CBR {

/** Interface for objects that need to listen for location updates. */
class LocationServiceListener {
public:
    virtual ~LocationServiceListener();

    virtual void localObjectAdded(const UUID& uuid) = 0;
    virtual void localObjectRemoved(const UUID& uuid) = 0;

    virtual void locationUpdated(const UUID& uuid, const TimedMotionVector3f& newval) = 0;
    virtual void boundsUpdated(const UUID& uuid, const BoundingSphere3f& newval) = 0;
}; // class LocationServiceListener

/** Interface for location services.  This provides a way for other components
 *  to get the most current information about object locations.
 */
class LocationService {
public:
    virtual ~LocationService();

    virtual void tick(const Time& t) = 0;

    /** Methods dealing with information requests. */
    virtual TimedMotionVector3f location(const UUID& uuid) = 0;
    virtual Vector3f currentPosition(const UUID& uuid) = 0;
    virtual BoundingSphere3f bounds(const UUID& uuid) = 0;

    /** Methods dealing with local objects. */
    virtual void addLocalObject(const UUID& uuid) = 0;
    virtual void removeLocalObject(const UUID& uuid) = 0;

    /** Methods dealing with listeners. */
    virtual void addListener(LocationServiceListener* listener);
    virtual void removeListener(LocationServiceListener* listener);

protected:
    void notifyLocationUpdated(const UUID& uuid, const TimedMotionVector3f& newval) const;
    void notifyBoundsUpdated(const UUID& uuid, const BoundingSphere3f& newval) const;

    typedef std::set<LocationServiceListener*> ListenerList;
    ListenerList mListeners;
}; // class LocationService

} // namespace CBR

#endif //_CBR_LOCATION_SERVICE_HPP_

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
#include "SpaceContext.hpp"
#include "MotionVector.hpp"
#include "Message.hpp"
#include "PollingService.hpp"

#include "SSTImpl.hpp"

namespace CBR {

class LocationServiceListener;
class LocationUpdatePolicy;
class LocationService;

/** Interface for objects that need to listen for location updates. */
class LocationServiceListener {
public:
    virtual ~LocationServiceListener();

    virtual void localObjectAdded(const UUID& uuid, const TimedMotionVector3f& loc, const BoundingSphere3f& bounds) = 0;
    virtual void localObjectRemoved(const UUID& uuid) = 0;
    virtual void localLocationUpdated(const UUID& uuid, const TimedMotionVector3f& newval) = 0;
    virtual void localBoundsUpdated(const UUID& uuid, const BoundingSphere3f& newval) = 0;

    virtual void replicaObjectAdded(const UUID& uuid, const TimedMotionVector3f& loc, const BoundingSphere3f& bounds) = 0;
    virtual void replicaObjectRemoved(const UUID& uuid) = 0;
    virtual void replicaLocationUpdated(const UUID& uuid, const TimedMotionVector3f& newval) = 0;
    virtual void replicaBoundsUpdated(const UUID& uuid, const BoundingSphere3f& newval) = 0;
}; // class LocationServiceListener


/** LocationUpdatePolicy controls when updates are sent to subscribers.
 *  It is informed of subscriptions, object location updates, and gets
 *  regular tick calls.  Based on this information it decides when to
 *  send updates to individual subscribers.
 */
class LocationUpdatePolicy : public LocationServiceListener{
public:
    LocationUpdatePolicy(LocationService* locservice);
    virtual ~LocationUpdatePolicy();

    virtual void subscribe(ServerID remote, const UUID& uuid) = 0;
    virtual void unsubscribe(ServerID remote, const UUID& uuid) = 0;
    virtual void unsubscribe(ServerID remote) = 0;

    virtual void subscribe(const UUID& remote, const UUID& uuid) = 0;
    virtual void unsubscribe(const UUID& remote, const UUID& uuid) = 0;
    virtual void unsubscribe(const UUID& remote) = 0;


    virtual void localObjectAdded(const UUID& uuid, const TimedMotionVector3f& loc, const BoundingSphere3f& bounds) = 0;
    virtual void localObjectRemoved(const UUID& uuid) = 0;
    virtual void localLocationUpdated(const UUID& uuid, const TimedMotionVector3f& newval) = 0;
    virtual void localBoundsUpdated(const UUID& uuid, const BoundingSphere3f& newval) = 0;

    virtual void replicaObjectAdded(const UUID& uuid, const TimedMotionVector3f& loc, const BoundingSphere3f& bounds) = 0;
    virtual void replicaObjectRemoved(const UUID& uuid) = 0;
    virtual void replicaLocationUpdated(const UUID& uuid, const TimedMotionVector3f& newval) = 0;
    virtual void replicaBoundsUpdated(const UUID& uuid, const BoundingSphere3f& newval) = 0;

    virtual void service() = 0;

protected:
    LocationService* mLocService; // The owner of this UpdatePolicy
    Router<Message*>* mLocMessageRouter; // Server Message Router for Loc Service
}; // class LocationUpdatePolicy


/** Interface for location services.  This provides a way for other components
 *  to get the most current information about object locations.
 */
class LocationService : public MessageRecipient, public ObjectMessageRecipient, public PollingService {
public:
    enum TrackingType {
        NotTracking,
        Local,
        Replica
    };

    LocationService(SpaceContext* ctx);
    virtual ~LocationService();

    const SpaceContext* context() const {
        return mContext;
    }

    virtual void newStream(boost::shared_ptr< Stream<UUID> > s) {
      boost::shared_ptr<Connection<UUID> > conn = s->connection().lock();

      UUID sourceObject = conn->remoteEndPoint().endPoint;	

      conn->registerReadDatagramCallback( OBJECT_PORT_LOCATION, 
					  std::tr1::bind(&LocationService::locationUpdate, this, sourceObject, _1, _2) );
    }

    /** Indicates whether this location service is tracking the given object.  It is only
     *  safe to request information */
    virtual bool contains(const UUID& uuid) const = 0;
    virtual TrackingType type(const UUID& uuid) const = 0;

    /** Methods dealing with information requests. */
    virtual TimedMotionVector3f location(const UUID& uuid) = 0;
    virtual Vector3f currentPosition(const UUID& uuid) = 0;
    virtual BoundingSphere3f bounds(const UUID& uuid) = 0;

    /** Methods dealing with local objects. */
    virtual void addLocalObject(const UUID& uuid, const TimedMotionVector3f& loc, const BoundingSphere3f& bounds) = 0;
    virtual void removeLocalObject(const UUID& uuid) = 0;

    /** Methods dealing with replica objects. */
    virtual void addReplicaObject(const Time& t, const UUID& uuid, const TimedMotionVector3f& loc, const BoundingSphere3f& bounds) = 0;
    virtual void removeReplicaObject(const Time& t, const UUID& uuid) = 0;

    /** Methods dealing with listeners. */
    virtual void addListener(LocationServiceListener* listener);
    virtual void removeListener(LocationServiceListener* listener);

    /** Subscriptions for other servers. */
    virtual void subscribe(ServerID remote, const UUID& uuid);
    virtual void unsubscribe(ServerID remote, const UUID& uuid);
    /** Unsubscripe the given server from all its location subscriptions. */
    virtual void unsubscribe(ServerID remote);

    /** Subscriptions for local objects. */
    virtual void subscribe(const UUID& remote, const UUID& uuid);
    virtual void unsubscribe(const UUID& remote, const UUID& uuid);
    /** Unsubscripe the given server from all its location subscriptions. */
    virtual void unsubscribe(const UUID& remote);


    /** MessageRecipient Interface. */
    virtual void receiveMessage(Message* msg) = 0;
    /** ObjectMessageRecipient Interface. */
    virtual void receiveMessage(const CBR::Protocol::Object::ObjectMessage& msg) = 0;

    virtual void locationUpdate(UUID source, void* buffer, uint length) = 0;

    boost::shared_ptr< Stream<UUID> > getObjectStream(const UUID& uuid) {
      return mContext->getObjectStream(uuid);
    }

protected:
    virtual void poll();
    virtual void service() = 0;

    void notifyLocalObjectAdded(const UUID& uuid, const TimedMotionVector3f& loc, const BoundingSphere3f& bounds) const;
    void notifyLocalObjectRemoved(const UUID& uuid) const;
    void notifyLocalLocationUpdated(const UUID& uuid, const TimedMotionVector3f& newval) const;
    void notifyLocalBoundsUpdated(const UUID& uuid, const BoundingSphere3f& newval) const;


    void notifyReplicaObjectAdded(const UUID& uuid, const TimedMotionVector3f& loc, const BoundingSphere3f& bounds) const;
    void notifyReplicaObjectRemoved(const UUID& uuid) const;
    void notifyReplicaLocationUpdated(const UUID& uuid, const TimedMotionVector3f& newval) const;
    void notifyReplicaBoundsUpdated(const UUID& uuid, const BoundingSphere3f& newval) const;

    SpaceContext* mContext;
private:
    TimeProfiler::Stage* mProfiler;
protected:
    typedef std::set<LocationServiceListener*> ListenerList;
    ListenerList mListeners;

    LocationUpdatePolicy* mUpdatePolicy;
}; // class LocationService

} // namespace CBR

#endif //_CBR_LOCATION_SERVICE_HPP_

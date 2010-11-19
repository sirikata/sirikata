/*  Sirikata
 *  Object.hpp
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

#ifndef _SIRIKATA_OBJECT_HPP_
#define _SIRIKATA_OBJECT_HPP_

#include <sirikata/core/util/Platform.hpp>

#include <sirikata/core/util/SpaceID.hpp>

#include <sirikata/core/network/ObjectMessage.hpp>
#include <sirikata/core/util/MotionPath.hpp>
#include <sirikata/core/network/SSTImpl.hpp>

#include <sirikata/core/util/SimpleExtrapolator.hpp>

#include <sirikata/core/odp/DelegateService.hpp>
#include <sirikata/core/odp/DelegatePort.hpp>

#include <boost/thread/shared_mutex.hpp>

#include <sirikata/oh/DisconnectCodes.hpp>

namespace Sirikata {

/** A property shared between multiple threads. Guarantees thread safety
 *  with a multi-reader, single writer lock.
 */
template<typename T>
class SharedProperty {
public:
    SharedProperty() {
        mutex = new boost::shared_mutex;
    }

    SharedProperty(const T& t)
     : data(t)
    {
        mutex = new boost::shared_mutex;
    }

    ~SharedProperty() {
        delete mutex;
    }

    T read() const {
        boost::shared_lock<boost::shared_mutex> lck(*mutex);
        return data;
    }

    void set(const T& t) {
        boost::unique_lock<boost::shared_mutex> lck(*mutex);
        data = t;
    }

    void operator=(const T& t) {
        set(t);
    }
private:
    T data;
    boost::shared_mutex* mutex;
}; // class SharedPropery


typedef std::set<UUID> ObjectSet;

class ObjectHostContext;
class ObjectFactory;

struct MaxDistUpdatePredicate {
    static float64 maxDist;
    bool operator()(const MotionVector3f& lhs, const MotionVector3f& rhs) const {
        return (lhs.position() - rhs.position()).length() > maxDist;
    }
};

class Object : public Service {
public:
    /** Standard constructor. */
    Object(ObjectFactory* obj_factory, const UUID& id, MotionPath* motion, const BoundingSphere3f& bnds, bool regQuery, SolidAngle queryAngle, const ObjectHostContext* ctx);

    ~Object();

    const UUID& uuid() const {
        return mID;
    }

    const TimedMotionVector3f location() const;
    const BoundingSphere3f bounds() const;

    void receiveMessage(const Sirikata::Protocol::Object::ObjectMessage* msg);

    virtual void start();
    virtual void stop();
    ServerID connectedTo()const {
        return mConnectedTo;
    }
    bool connected();
    bool send(uint16 src_port, UUID dest, uint16 dest_port, std::string payload);
private:
    typedef EndPoint<SpaceObjectReference> EndPointType;
    typedef BaseDatagramLayer<SpaceObjectReference> BaseDatagramLayerType;
    typedef BaseDatagramLayerType::Ptr BaseDatagramLayerPtr;
    typedef Stream<SpaceObjectReference> SSTStream;
    typedef SSTStream::Ptr SSTStreamPtr;
    typedef Connection<SpaceObjectReference> SSTConnection;
    typedef SSTConnection::Ptr SSTConnectionPtr;

    // Initiate a connection
    void connect();
    // Disconnects from the space if a connection has been established
    void disconnect();

    void scheduleNextLocUpdate();
    void handleNextLocUpdate(const TimedMotionVector3f& up);

    // Handlers for substreams for space-managed updates
    void handleLocationSubstream(int err, SSTStreamPtr s);
    void handleProximitySubstream(int err, SSTStreamPtr s);
    // Handlers for substream read events for space-managed updates
    void handleLocationSubstreamRead(SSTStreamPtr s, std::stringstream* prevdata, uint8* buffer, int length);
    void handleProximitySubstreamRead(SSTStreamPtr s, std::stringstream* prevdata, uint8* buffer, int length);

    bool locationMessage(const std::string& payload);
    bool proximityMessage(const std::string& payload);

    // Handle a new connection to a space -- initiate session
    void handleSpaceConnection(const SpaceID& space, const ObjectReference&, ServerID sid);
    // We need to manually wrap this for the main strand because IOStrand
    // doesn't support > 5 arguments, which the original callback has
    void handleSpaceConnectionIndirect(const SpaceID& space, const ObjectReference&, ServerID sid);
    // Handle a migration to a new space server
    void handleSpaceMigration(const SpaceID& space, const ObjectReference&, ServerID sid);
    void handleSpaceStreamCreated();
    // Handle a disconnection from the space
    void handleSpaceDisconnection(const SpaceObjectReference& spaceobj, Disconnect::Code);

    void sendNoReturn(uint16 src_port, UUID dest, uint16 dest_port, std::string payload);


    // ODP::Service Interface
    virtual ODP::Port* bindODPPort(const SpaceID& space, const ObjectReference& objref, ODP::PortID port);
    virtual ODP::Port* bindODPPort(const SpaceObjectReference& sor, ODP::PortID port);
    virtual ODP::Port* bindODPPort(const SpaceID& space, const ObjectReference& objref);
    virtual ODP::Port* bindODPPort(const SpaceObjectReference& sor);
    virtual void registerDefaultODPHandler(const ODP::MessageHandler& cb);
    ODP::DelegatePort* createDelegateODPPort(ODP::DelegateService* parentService, const SpaceObjectReference& spaceobj, ODP::PortID port);
    bool delegateODPPortSend(const ODP::Endpoint& source_ep, const ODP::Endpoint& dest_ep, MemoryReference payload);

    // THREAD SAFE:
    // These are thread safe (they don't change after initialization)
    const UUID mID;
    const ObjectHostContext* const mContext;
    ObjectFactory* const mObjectFactory;

    // LOCK PROTECTED:
    // These need to be accessed by multiple threads, protected by locks
    SharedProperty<BoundingSphere3f> mBounds; // FIXME Should probably be variable
    SharedProperty<TimedMotionVector3f> mLocation;

    // OBJECT THREAD:
    // Only accessed by object simulation operations
    MotionPath* mMotion;
    SimpleExtrapolator<MotionVector3f, MaxDistUpdatePredicate> mLocationExtrapolator;
    bool mRegisterQuery;
    SolidAngle mQueryAngle;

    ServerID mConnectedTo;
    bool mMigrating;
    bool mQuitting;

    Network::IOTimerPtr mLocUpdateTimer;

    ODP::DelegateService* mDelegateODPService;
    BaseDatagramLayerPtr mSSTDatagramLayer;
}; // class Object

} // namespace Sirikata

#endif //_SIRIKATA_OBJECT_HPP_

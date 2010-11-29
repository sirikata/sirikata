/*  Sirikata
 *  Object.cpp
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

#include "Object.hpp"
#include <sirikata/core/network/ObjectMessage.hpp>
#include <sirikata/core/util/Random.hpp>
#include <sirikata/oh/ObjectHostContext.hpp>
#include "ObjectHost.hpp"
#include "ObjectFactory.hpp"
#include <sirikata/core/trace/Trace.hpp>
#include <sirikata/oh/Trace.hpp>
#include <sirikata/core/network/IOStrandImpl.hpp>
#include <boost/bind.hpp>

#include "Protocol_Frame.pbj.hpp"
#include "Protocol_Prox.pbj.hpp"
#include "Protocol_Loc.pbj.hpp"


#define OBJ_LOG(level,msg) SILOG(object,level,"[OBJ] " << msg)

namespace Sirikata {

float64 MaxDistUpdatePredicate::maxDist = 3.0;

Object::Object(ObjectFactory* obj_factory, const UUID& id, MotionPath* motion, const BoundingSphere3f& bnds, bool regQuery, SolidAngle queryAngle, const ObjectHostContext* ctx)
 : mID(id),
   mContext(ctx),
   mObjectFactory(obj_factory),
   mBounds(bnds),
   mLocation(motion->initial()),
   mMotion(motion),
   mLocationExtrapolator(mMotion->initial(), MaxDistUpdatePredicate()),
   mRegisterQuery(regQuery),
   mQueryAngle(queryAngle),
   mConnectedTo(0),
   mMigrating(false),
   mQuitting(false),
   mLocUpdateTimer( Network::IOTimer::create(mContext->ioService) )
{
    mDelegateODPService = new ODP::DelegateService(
        std::tr1::bind(
            &Object::createDelegateODPPort, this,
            _1, _2, _3
        )
    );

    mSSTDatagramLayer = BaseDatagramLayerType::createDatagramLayer(SpaceObjectReference(SpaceID::null(), ObjectReference(id)), ctx, mDelegateODPService);
}

Object::~Object() {
    stop();
    disconnect();
    mObjectFactory->notifyDestroyed(mID);
}

void Object::start() {
    connect();
}

void Object::stop() {
    mQuitting = true;
    mLocUpdateTimer->cancel();
}

void Object::scheduleNextLocUpdate() {
    const Time tnow = mContext->simTime();

    TimedMotionVector3f curLoc = location();
    const TimedMotionVector3f* update = mMotion->nextUpdate(tnow);
    if (update != NULL) {

        mLocUpdateTimer->wait(
            update->time() - tnow,
            mContext->mainStrand->wrap(
                std::tr1::bind(&Object::handleNextLocUpdate, this, *update)
            )
        );
    }
}

void Object::handleNextLocUpdate(const TimedMotionVector3f& up) {
    if (mQuitting) {
        disconnect();
        return;
    }

    const Time tnow = mContext->simTime();

    TimedMotionVector3f curLoc = up;
    mLocation = curLoc;

    if (!mMigrating && mLocationExtrapolator.needsUpdate(tnow, curLoc.extrapolate(tnow))) {
        mLocationExtrapolator.updateValue(curLoc.time(), curLoc.value());

        // Generate and send an update to Loc
        Sirikata::Protocol::Loc::Container container;
        Sirikata::Protocol::Loc::ILocationUpdateRequest loc_request = container.mutable_update_request();
        Sirikata::Protocol::ITimedMotionVector requested_loc = loc_request.mutable_location();
        requested_loc.set_t(curLoc.updateTime());
        requested_loc.set_position(curLoc.position());
        requested_loc.set_velocity(curLoc.velocity());

        std::string payload = serializePBJMessage(container);

	SSTStreamPtr spaceStream = mContext->objectHost->getSpaceStream(mID);
        if (spaceStream != SSTStreamPtr()) {
          SSTConnectionPtr conn = spaceStream->connection().lock();
          assert(conn);

          conn->datagram( (void*)payload.data(), payload.size(), OBJECT_PORT_LOCATION,
                          OBJECT_PORT_LOCATION, NULL);
	}

        // XXX FIXME do something on failure
        CONTEXT_OHTRACE_NO_TIME(objectGenLoc, tnow, mID, curLoc, bounds());
    }

    scheduleNextLocUpdate();
}

bool Object::send(uint16 src_port, UUID dest, uint16 dest_port, std::string payload) {
  bool val = mContext->objectHost->send(
      this, src_port,
      dest, dest_port,
      payload
  );

  return val;
}
void Object::sendNoReturn(uint16 src_port, UUID dest, uint16 dest_port, std::string payload) {
    send(src_port, dest, dest_port, payload);
}

const TimedMotionVector3f Object::location() const {
    return mLocation.read();
}

const BoundingSphere3f Object::bounds() const {
    return mBounds.read();
}

void Object::connect() {
    if (connected()) {
        OBJ_LOG(warning,"Tried to connect when already connected " << mID.toString());
        return;
    }

    TimedMotionVector3f curMotion = mMotion->at(mContext->simTime());

    using std::tr1::placeholders::_1;
    using std::tr1::placeholders::_2;

    if (mRegisterQuery)
        mContext->objectHost->connect(
            this,
            mQueryAngle,
            std::tr1::bind(&Object::handleSpaceConnection, this, _1, _2, _3),
            mContext->mainStrand->wrap( std::tr1::bind(&Object::handleSpaceMigration, this, _1, _2, _3) ),
	    mContext->mainStrand->wrap( std::tr1::bind(&Object::handleSpaceStreamCreated, this) ),
            mContext->mainStrand->wrap( std::tr1::bind(&Object::handleSpaceDisconnection, this, _1, _2) )
        );
    else
        mContext->objectHost->connect(
            this,
            std::tr1::bind(&Object::handleSpaceConnection, this, _1, _2, _3),
            mContext->mainStrand->wrap( std::tr1::bind(&Object::handleSpaceMigration, this, _1, _2, _3) ),
	    mContext->mainStrand->wrap( std::tr1::bind(&Object::handleSpaceStreamCreated, this ) ),
            mContext->mainStrand->wrap( std::tr1::bind(&Object::handleSpaceDisconnection, this, _1, _2) )
        );
}

void Object::disconnect() {
    // FIXME send if a connection has been requested but not completed
    if (connected())
        mContext->objectHost->disconnect(this);
}

void Object::handleSpaceConnection(const SpaceID& space, const ObjectReference& obj, ServerID sid) {
    // We need to manually wrap this for the main strand because IOStrand
    // doesn't support > 5 arguments, which the original callback has
    mContext->mainStrand->post(
        std::tr1::bind(&Object::handleSpaceConnectionIndirect, this, space, obj, sid)
    );
}
void Object::handleSpaceConnectionIndirect(const SpaceID& space, const ObjectReference& obj, ServerID sid) {
    if (sid == 0) {
        OBJ_LOG(error,"Failed to open connection for object " << mID.toString());
        return;
    }

    OBJ_LOG(insane,"Got space connection callback");
    mConnectedTo = sid;

    // Always record our initial position, may be the only "update" we ever send
    const Time tnow = mContext->simTime();
    TimedMotionVector3f curLoc = location();
    BoundingSphere3f curBounds = bounds();
    CONTEXT_OHTRACE_NO_TIME(objectConnected, tnow, mID, sid);
    CONTEXT_OHTRACE_NO_TIME(objectGenLoc, tnow, mID, curLoc, curBounds);

    // Start normal processing
    mContext->mainStrand->post(
        std::tr1::bind(&Object::scheduleNextLocUpdate, this)
    );
}

void Object::handleSpaceMigration(const SpaceID& space, const ObjectReference& obj, ServerID sid) {
    OBJ_LOG(insane,"Migrated to new space server: " << sid);
    mConnectedTo = sid;
}

void Object::handleSpaceStreamCreated() {
    SSTStreamPtr sstStream = mContext->objectHost->getSpaceStream(mID);
  using std::tr1::placeholders::_1;
  using std::tr1::placeholders::_2;

  if (sstStream) {
      sstStream->listenSubstream(OBJECT_PORT_LOCATION,
          std::tr1::bind(&Object::handleLocationSubstream, this, _1, _2)
      );
      sstStream->listenSubstream(OBJECT_PORT_PROXIMITY,
          std::tr1::bind(&Object::handleProximitySubstream, this, _1, _2)
      );
  }
}

void Object::handleSpaceDisconnection(const SpaceObjectReference& spaceobj, Disconnect::Code) {
}

bool Object::connected() {
    return (mConnectedTo != NullServerID);
}

void Object::receiveMessage(const Sirikata::Protocol::Object::ObjectMessage* msg) {
    assert( msg->dest_object() == uuid() );

    mDelegateODPService->deliver(
        ODP::Endpoint(SpaceID::null(), ObjectReference(msg->source_object()), msg->source_port()),
        ODP::Endpoint(SpaceID::null(), ObjectReference(msg->dest_object()), msg->dest_port()),
        MemoryReference(msg->payload())
    );
    delete msg;
}

// ODP::Service Interface
ODP::Port* Object::bindODPPort(const SpaceID& space, const ObjectReference& objref, ODP::PortID port) {
    return mDelegateODPService->bindODPPort(space, objref, port);
}

ODP::Port* Object::bindODPPort(const SpaceObjectReference& sor, ODP::PortID port) {
    return mDelegateODPService->bindODPPort(sor, port);
}

ODP::Port* Object::bindODPPort(const SpaceID& space, const ObjectReference& objref) {
    return mDelegateODPService->bindODPPort(space, objref);
}

ODP::Port* Object::bindODPPort(const SpaceObjectReference& sor) {
    return mDelegateODPService->bindODPPort(sor);
}

void Object::registerDefaultODPHandler(const ODP::MessageHandler& cb) {
    mDelegateODPService->registerDefaultODPHandler(cb);
}

ODP::DelegatePort* Object::createDelegateODPPort(ODP::DelegateService* parentService, const SpaceObjectReference& spaceobj, ODP::PortID port) {
    ODP::Endpoint port_ep(spaceobj, port);
    return new ODP::DelegatePort(
        mDelegateODPService,
        port_ep,
        std::tr1::bind(
            &Object::delegateODPPortSend, this,
            port_ep, _1, _2
        )
    );
}

bool Object::delegateODPPortSend(const ODP::Endpoint& source_ep, const ODP::Endpoint& dest_ep, MemoryReference payload) {
    assert(source_ep.space() == dest_ep.space());
    return send((uint16)source_ep.port(), dest_ep.object().getAsUUID(), (uint16)dest_ep.port(), String((const char*)payload.data(), payload.size()));
}

void Object::handleLocationSubstream(int err, SSTStreamPtr s) {
    s->registerReadCallback( std::tr1::bind(&Object::handleLocationSubstreamRead, this, s, new std::stringstream(), _1, _2) );
}

void Object::handleProximitySubstream(int err, SSTStreamPtr s) {
    s->registerReadCallback( std::tr1::bind(&Object::handleProximitySubstreamRead, this, s, new std::stringstream(), _1, _2) );
}

void Object::handleLocationSubstreamRead(SSTStreamPtr s, std::stringstream* prevdata, uint8* buffer, int length) {
    prevdata->write((const char*)buffer, length);
    if (locationMessage(prevdata->str())) {
        // FIXME we should be getting a callback on stream close instead of
        // relying on this parsing as an indicator
        delete prevdata;
        // Clear out callback so we aren't responsible for any remaining
        // references to s
        s->registerReadCallback(0);
    }
}

void Object::handleProximitySubstreamRead(SSTStreamPtr s, std::stringstream* prevdata, uint8* buffer, int length) {
    prevdata->write((const char*)buffer, length);
    if (proximityMessage(prevdata->str())) {
        // FIXME we should be getting a callback on stream close instead of
        // relying on this parsing as an indicator
        delete prevdata;
        // Clear out callback so we aren't responsible for any remaining
        // references to s
        s->registerReadCallback(0);
    }
}


bool Object::locationMessage(const std::string& payload) {
    Sirikata::Protocol::Frame frame;
    bool parse_success = frame.ParseFromString(payload);
    if (!parse_success) return false;
    Sirikata::Protocol::Loc::BulkLocationUpdate contents;
    parse_success = contents.ParseFromString(frame.payload());
    assert(parse_success);

    for(int32 idx = 0; idx < contents.update_size(); idx++) {
        Sirikata::Protocol::Loc::LocationUpdate update = contents.update(idx);

        Sirikata::Protocol::TimedMotionVector update_loc = update.location();
        TimedMotionVector3f loc(update_loc.t(), MotionVector3f(update_loc.position(), update_loc.velocity()));

        CONTEXT_OHTRACE(objectLoc,
            mID,
            update.object(),
            loc
        );

        // FIXME do something with the data
    }
    return true;
}

bool Object::proximityMessage(const std::string& payload) {
    //assert(msg.source_object() == UUID::null()); // Should originate at space
    //server
    Sirikata::Protocol::Frame frame;
    bool parse_success = frame.ParseFromString(payload);
    if (!parse_success) return false;
    Sirikata::Protocol::Prox::ProximityResults contents;
    parse_success = contents.ParseFromString(frame.payload());
    assert(parse_success);

    for(int32 idx = 0; idx < contents.update_size(); idx++) {
        Sirikata::Protocol::Prox::ProximityUpdate update = contents.update(idx);

        for(int32 aidx = 0; aidx < update.addition_size(); aidx++) {
            Sirikata::Protocol::Prox::ObjectAddition addition = update.addition(aidx);

            TimedMotionVector3f loc(addition.location().t(), MotionVector3f(addition.location().position(), addition.location().velocity()));

            CONTEXT_OHTRACE(prox,
                mID,
                addition.object(),
                true,
                loc
            );
        }

        for(int32 ridx = 0; ridx < update.removal_size(); ridx++) {
            Sirikata::Protocol::Prox::ObjectRemoval removal = update.removal(ridx);

            CONTEXT_OHTRACE(prox,
                mID,
                removal.object(),
                false,
                TimedMotionVector3f()
            );
        }
    }

    return true;
}

} // namespace Sirikata

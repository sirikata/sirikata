/*  cbr
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

#include "Object.hpp"
#include "Message.hpp"
#include "Random.hpp"
#include "ObjectHostContext.hpp"
#include "ObjectHost.hpp"
#include "ObjectFactory.hpp"
#include "Statistics.hpp"
#include <network/IOStrandImpl.hpp>
#include <boost/bind.hpp>

#include "CBR_Prox.pbj.hpp"
#include "CBR_Loc.pbj.hpp"


#define OBJ_LOG(level,msg) SILOG(object,level,"[OBJ] " << msg)

namespace CBR {

void datagramSendDoneCallback(int, void*) {

}

void datagramCallback(uint8* buffer, int length) {
  printf( "Received datagram %s\n", buffer );
}


void streamReadCallback(uint8* buffer, int length) {
  //printf( "Received datagram %s\n", buffer );
}


class ReadClass {
public:
 int numBytesReceived;

 ReadClass():numBytesReceived(0) {}

 void childStreamReadCallback(uint8* buffer, int length) {
   

   for (int i=0; i<length; i++) {    
     assert( buffer[i] == (i+numBytesReceived) % 255  );
   }

   numBytesReceived += length;

   printf("numBytesReceived=%d in object %x\n", numBytesReceived, ((uint8*)this));
  
    fflush(stdout);
 }
};


void func4(int err, boost::shared_ptr< Stream<UUID> > s) {
  if (err != 0) {
    printf("stream could not be successfully created\n");
    return;
  }

  int length = 1000000;

  uint8* f = new uint8[length];


  for (int i=0; i<length; i++) {
    f[i] = i % 255;
  }

  s->write(f, length);
}

void delayed_register( boost::shared_ptr< Stream<UUID> > s) {
  boost::this_thread::sleep( boost::posix_time::milliseconds(1000) );

  ReadClass *c = new ReadClass();
  
  s->registerReadCallback( std::tr1::bind( &ReadClass::childStreamReadCallback, c, _1, _2)  ) ;
}


void func3(int err, boost::shared_ptr< Stream<UUID> > s) {
  if (err != 0) return;

  s->registerReadCallback(streamReadCallback);

  printf("func3 called\n");

  s->connection().lock()->registerReadDatagramCallback(datagramCallback);

  for (int i=0 ;i<1000; i++) {
    s->connection().lock()->datagram( (uint8*)("foobar\n\0"), 8, datagramSendDoneCallback );
  }
  
  s->createChildStream( func4,  NULL, 0  );  
}

void func5(int err, boost::shared_ptr< Stream<UUID> > s) {
  std::cout << "func5 called with endpoint " << s->connection().lock()->localEndPoint().endPoint.toString() << "\n";
  
  //s->registerReadCallback( childStreamReadCallback  );
  s->connection().lock()->registerReadDatagramCallback(datagramCallback);

  new Thread(boost::bind(delayed_register, s));
}



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
   mLocUpdateTimer( IOTimer::create(mContext->ioService) )
{
  mSSTDatagramLayer = BaseDatagramLayer<UUID>::createDatagramLayer(mID, this, this);
  
}

Object::~Object() {
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
        CBR::Protocol::Loc::Container container;
        CBR::Protocol::Loc::ILocationUpdateRequest loc_request = container.mutable_update_request();
        CBR::Protocol::Loc::ITimedMotionVector requested_loc = loc_request.mutable_location();
        requested_loc.set_t(curLoc.updateTime());
        requested_loc.set_position(curLoc.position());
        requested_loc.set_velocity(curLoc.velocity());

	bool success = mContext->objectHost->send(
            this, OBJECT_PORT_LOCATION,
            UUID::null(), OBJECT_PORT_LOCATION,
            serializePBJMessage(container)
        );


        // XXX FIXME do something on failure
        mContext->trace()->objectGenLoc(tnow, mID, curLoc);
    }

    scheduleNextLocUpdate();
}

bool Object::send( uint16 src_port,  UUID src,  uint16 dest_port,  UUID dest, std::string payload) {
  bool val = mContext->objectHost->send(
			     src_port, src,
			     dest_port, dest,
			     payload
			     );

  std::cout << "Sending... " << val << "\n";
  fflush(stdout);

  return val;
}

bool Object::route(CBR::Protocol::Object::ObjectMessage* msg) {
  mContext->mainStrand->post(std::tr1::bind(
			     &Object::send, this,
			     msg->source_port(), msg->source_object(),
			     msg->dest_port(), msg->dest_object(),
			     msg->payload())
			    );
  
  delete msg;

  return true;
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

    if (mRegisterQuery)
        mContext->objectHost->connect(
            this,
            mQueryAngle,
            mContext->mainStrand->wrap( boost::bind(&Object::handleSpaceConnection, this, _1) ),
            mContext->mainStrand->wrap( boost::bind(&Object::handleSpaceMigration, this, _1) )
        );
    else
        mContext->objectHost->connect(
            this,
            mContext->mainStrand->wrap( boost::bind(&Object::handleSpaceConnection, this, _1) ),
            mContext->mainStrand->wrap( boost::bind(&Object::handleSpaceMigration, this, _1) )
        );
}

void Object::disconnect() {
    // FIXME send if a connection has been requested but not completed
    if (connected())
        mContext->objectHost->disconnect(this);
}

void Object::handleSpaceConnection(ServerID sid) {
    if (sid == 0) {
        OBJ_LOG(error,"Failed to open connection for object " << mID.toString());
        return;
    }

    OBJ_LOG(insane,"Got space connection callback");
    mConnectedTo = sid;

    // Start normal processing
    mContext->mainStrand->post(
        std::tr1::bind(&Object::scheduleNextLocUpdate, this)
    );
}

void Object::handleSpaceMigration(ServerID sid) {
    OBJ_LOG(insane,"Migrated to new space server: " << sid);
    mConnectedTo = sid;
}

bool Object::connected() {
    return (mConnectedTo != NullServerID);
}

void Object::receiveMessage(const CBR::Protocol::Object::ObjectMessage* msg) {
    assert( msg->dest_object() == uuid() );

    switch( msg->dest_port() ) {
      case OBJECT_PORT_PROXIMITY:
        proximityMessage(*msg);
        break;
      case OBJECT_PORT_LOCATION:
        locationMessage(*msg);
        break;
      default:
        //printf("Warning: Tried to deliver unknown message type through ObjectConnection.\n");
        dispatchMessage(*msg);

        break;
    }

    delete msg;
}

void Object::locationMessage(const CBR::Protocol::Object::ObjectMessage& msg) {
    assert(msg.source_object() == UUID::null()); // Should come from space

    CBR::Protocol::Loc::BulkLocationUpdate contents;
    bool parse_success = contents.ParseFromString(msg.payload());
    assert(parse_success);

    for(int32 idx = 0; idx < contents.update_size(); idx++) {
        CBR::Protocol::Loc::LocationUpdate update = contents.update(idx);

        CBR::Protocol::Loc::TimedMotionVector update_loc = update.location();
        TimedMotionVector3f loc(update_loc.t(), MotionVector3f(update_loc.position(), update_loc.velocity()));

        mContext->trace()->objectLoc(
            mContext->simTime(),
            msg.dest_object(),
            update.object(),
            loc
        );

        // FIXME do something with the data
    }
}

void Object::proximityMessage(const CBR::Protocol::Object::ObjectMessage& msg) {
    assert(msg.source_object() == UUID::null()); // Should originate at space server
    CBR::Protocol::Prox::ProximityResults contents;
    bool parse_success = contents.ParseFromString(msg.payload());
    assert(parse_success);

    for(int32 idx = 0; idx < contents.addition_size(); idx++) {
        CBR::Protocol::Prox::ObjectAddition addition = contents.addition(idx);

        TimedMotionVector3f loc(addition.location().t(), MotionVector3f(addition.location().position(), addition.location().velocity()));

        mContext->trace()->prox(
            mContext->simTime(),
            msg.dest_object(),
            addition.object(),
            true,
            loc
        );

	if (mID.toString() == "255d0517-58e9-5ed4-abb2-cdc69bb45411" &&
	    addition.object().toString() == "fbfaaa3a-fb29-d1e6-053c-7c9475d8be61")
	{
	  
	  
	  Stream<UUID>::connectStream(this, 
				      EndPoint<UUID>(mID, 50000), 
				      EndPoint<UUID>(addition.object(), 51000), 
				      func3
				      );
	}

	
    }

    

    for(int32 idx = 0; idx < contents.removal_size(); idx++) {
        CBR::Protocol::Prox::ObjectRemoval removal = contents.removal(idx);

        mContext->trace()->prox(
            mContext->simTime(),
            msg.dest_object(),
            removal.object(),
            false,
            TimedMotionVector3f()
        );
    }
}

} // namespace CBR

/*  cbr
 *  Server.cpp
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
#include "Network.hpp"
#include "Server.hpp"
#include "Proximity.hpp"
#include "Object.hpp"
#include "ObjectFactory.hpp"
#include "CoordinateSegmentation.hpp"
#include "Message.hpp"
#include "ServerIDMap.hpp"
#include "ServerMessageQueue.hpp"
#include "Statistics.hpp"
#include "Options.hpp"
#include "ObjectMessageQueue.hpp"
namespace CBR {

Server::Server(ServerID id, ObjectFactory* obj_factory, LocationService* loc_service, CoordinateSegmentation* cseg, Proximity* prox, ObjectMessageQueue* omq, ServerMessageQueue* smq, Trace* trace)
 : mID(id),
   mObjectFactory(obj_factory),
   mLocationService(loc_service),
   mCSeg(cseg),
   mProximity(prox),
   mObjectMessageQueue(omq),
   mServerMessageQueue(smq),
   mCurrentTime(0),
   mTrace(trace)
{
    // setup object which are initially residing on this server
    for(ObjectFactory::iterator it = mObjectFactory->begin(); it != mObjectFactory->end(); it++) {
        UUID obj_id = *it;
        Vector3f start_pos = loc_service->currentPosition(obj_id);

        if (lookup(start_pos) == mID) {
            // Instantiate object
            Object* obj = mObjectFactory->object(obj_id, this->id());
            mObjects[obj_id] = obj;
            // Register proximity query
            mProximity->addQuery(obj_id, 100.f); // FIXME how to set proximity radius?
        }
    }
}

Server::~Server() {
}

const ServerID& Server::id() const {
    return mID;
}

void Server::route(Message* msg, const ServerID& dest_server, bool is_forward) {
    assert(msg != NULL);

    uint32 offset = 0;
    Network::Chunk msg_serialized;
    offset = msg->serialize(msg_serialized, offset);

    if (dest_server==id()) {
        if (!is_forward) {
            mTrace->packetQueued(mCurrentTime, dest_server, msg->id(), offset);
            mTrace->packetSent(mCurrentTime, mCurrentTime, dest_server, msg->id(), offset); // self rate is infinite => start and end times are identical
        }

        mSelfMessages.push_back( SelfMessage(msg_serialized, is_forward) );
    }else {
        mTrace->packetQueued(mCurrentTime, dest_server, msg->id(), offset);

        mOutgoingMessages.push_back( OutgoingMessage(msg_serialized, dest_server) );
    }
    delete msg;
}

void Server::route(Message* msg, const UUID& dest_obj, bool is_forward) {
    ServerID dest_server_id = lookup(dest_obj);
    route(msg, dest_server_id, is_forward);
}

void Server::deliver(Message* msg) {
    switch(msg->type()) {
      case MESSAGE_TYPE_PROXIMITY:
          {
              ProximityMessage* prox_msg = dynamic_cast<ProximityMessage*>(msg);
              assert(prox_msg != NULL);

              Object* dest_obj = object(prox_msg->destObject());
              if (dest_obj == NULL)
                  forward(prox_msg, prox_msg->destObject());
              else {
                  mTrace->prox(mCurrentTime, prox_msg->destObject(), prox_msg->neighbor(), (prox_msg->event() == ProximityMessage::Entered) ? true : false, prox_msg->location() );
                  dest_obj->proximityMessage(prox_msg);
              }
          }
          break;
      case MESSAGE_TYPE_LOCATION:
          {
              LocationMessage* loc_msg = dynamic_cast<LocationMessage*>(msg);
              assert(loc_msg != NULL);

              Object* dest_obj = object(loc_msg->destObject());
              if (dest_obj == NULL)
                  forward(loc_msg, loc_msg->destObject());
              else {
                  mTrace->loc(mCurrentTime, loc_msg->destObject(), loc_msg->sourceObject(), loc_msg->location());
                  dest_obj->locationMessage(loc_msg);
              }
          }
          break;
      case MESSAGE_TYPE_SUBSCRIPTION:
          {
              SubscriptionMessage* subs_msg = dynamic_cast<SubscriptionMessage*>(msg);
              assert(subs_msg != NULL);

              Object* dest_obj = object(subs_msg->destObject());
              if (dest_obj == NULL)
                  forward(subs_msg, subs_msg->destObject());
              else {
                  mTrace->subscription(mCurrentTime, subs_msg->destObject(), subs_msg->sourceObject(), (subs_msg->action() == SubscriptionMessage::Subscribe) ? true : false);
                  dest_obj->subscriptionMessage(subs_msg);
              }
          }
        break;
      case MESSAGE_TYPE_MIGRATE:
          {
              MigrateMessage* migrate_msg = dynamic_cast<MigrateMessage*>(msg);
              assert(migrate_msg != NULL);

	      const UUID obj_id = migrate_msg->object();

	      Object* obj = mObjectFactory->object(obj_id, this->id());
	      obj->migrateMessage(migrate_msg);

	      mObjects[obj_id] = obj;

	      //printf("MESSAGE_TYPE_MIGRATE received from obj_id %s\n", obj_id.readableHexData().c_str());

              delete migrate_msg;
          }
          break;
      default:
        assert(false);
        break;
    }
}

Object* Server::object(const UUID& dest) const {
    ObjectMap::const_iterator it = mObjects.find(dest);
    if (it == mObjects.end())
        return NULL;
    return it->second;
}

void Server::forward(Message* msg, const UUID& dest) {
    ServerID dest_server_id = lookup(dest);
    route(msg, dest_server_id, true);
}

void Server::processChunk(const Network::Chunk&chunk, const ServerID& source_server, bool forwarded_self_msg) {
    Message* result;
    unsigned int offset=0;
    do {
        offset = Message::deserialize(chunk,offset,&result);

        if (!forwarded_self_msg)
            mTrace->packetReceived(mCurrentTime, mCurrentTime, source_server, result->id(), offset);

        deliver(result);
    }while (offset<chunk.size());
}
void Server::networkTick(const Time&t) {
    std::deque<SelfMessage> self_messages;
    self_messages.swap( mSelfMessages );
    while (!self_messages.empty()) {
        processChunk(self_messages.front().data, this->id(), self_messages.front().forwarded);
        self_messages.pop_front();
    }

    while(!mOutgoingMessages.empty()) {
        OutgoingMessage& next_msg = mOutgoingMessages.front();
        bool send_success = mServerMessageQueue->addMessage(next_msg.dest, next_msg.data);
        if (!send_success) break;
        mOutgoingMessages.pop_front();
    }

    mObjectMessageQueue->service(t);
    mServerMessageQueue->service(t);


    Sirikata::Network::Chunk *c=NULL;
    ServerID source_server;
    while(mServerMessageQueue->receive(&c, &source_server)) {
        processChunk(*c, source_server, false);
        delete c;
    }
}
void Server::tick(const Time& t) {
    mCurrentTime = t;

    // Update object locations
    mLocationService->tick(t);

    // Check proximity updates
    proximityTick(t);
    networkTick(t);
    // Check for object migrations
    checkObjectMigrations();

    // Give objects a chance to process
    for(ObjectMap::iterator it = mObjects.begin(); it != mObjects.end(); it++) {
        Object* obj = it->second;
        obj->tick(t);
    }
}

void Server::proximityTick(const Time& t) {
    // Check for proximity updates
    std::queue<ProximityEventInfo> proximity_events;
    mProximity->evaluate(t, proximity_events);

    OriginID origin;
    origin.id = (uint32)id();

    while(!proximity_events.empty()) {
        ProximityEventInfo& evt = proximity_events.front();
        ProximityMessage* msg =
            new ProximityMessage(
                origin,
                evt.query(),
                evt.object(),
                (evt.type() == ProximityEventInfo::Entered) ? ProximityMessage::Entered : ProximityMessage::Exited,
                mLocationService->location(evt.query())
            );
        route(msg, evt.query());

        proximity_events.pop();
    }
}

void Server::checkObjectMigrations() {
    // * check for objects crossing server boundaries
    // * wrap up state and send message to other server
    //     to reinstantiate the object there
    // * delete object on this side

    std::vector<UUID> migrated_objects;
    for(ObjectMap::iterator it = mObjects.begin(); it != mObjects.end(); it++) {
        Object* obj = it->second;
        const UUID& obj_id = obj->uuid();

        Vector3f obj_pos = mLocationService->currentPosition(obj_id);
	ServerID new_server_id = lookup(obj_pos);

        if (new_server_id != mID) {
	    MigrateMessage* migrate_msg = wrapObjectStateForMigration(obj);
/*
	    printf("migrating object %s due to position %s \n",
		   obj_id.readableHexData().c_str(),
		   obj_pos.toString().c_str());
*/
  	    route( migrate_msg , new_server_id);
	    migrated_objects.push_back(obj_id);
        }
    }

    for (std::vector<UUID>::iterator it = migrated_objects.begin(); it != migrated_objects.end(); it++){
      mObjects.erase(*it);
    }
}

MigrateMessage* Server::wrapObjectStateForMigration(Object* obj) {
    const UUID& obj_id = obj->uuid();

    OriginID origin;
    origin.id = (uint32)id();

    MigrateMessage* migrate_msg = new MigrateMessage(origin,
                                                    obj_id,
						    obj->proximityRadius(),
						    obj->subscriberSet().size());
    ObjectSet::iterator it;
    int i=0;
    UUID* migrate_msg_subscribers = migrate_msg->subscriberList();
    for (it = obj->subscriberSet().begin(); it != obj->subscriberSet().end(); it++) {
        migrate_msg_subscribers[i] = *it;
	//printf("sent migrateMsg->msubscribers[i] = %s\n",
	//       migrate_msg_subscribers[i].readableHexData().c_str()  );
        i++;
    }

    return migrate_msg;
}

ServerID Server::lookup(const Vector3f& pos) {
    ServerID sid = mCSeg->lookup(pos);
    return sid;
}

ServerID Server::lookup(const UUID& obj_id) {
    Vector3f pos = mLocationService->currentPosition(obj_id);
  ServerID sid = mCSeg->lookup(pos);
  return sid;
}

} // namespace CBR

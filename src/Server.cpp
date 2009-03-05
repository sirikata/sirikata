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
#include "ServerMap.hpp"
#include "Message.hpp"
#include "ServerIDMap.hpp"
#include "SendQueue.hpp"
namespace CBR {

Server::Server(ServerID id, ObjectFactory* obj_factory, LocationService* loc_service, ServerMap* server_map, Proximity* prox, Network* net, SendQueue * sq)
 : mID(id),
   mObjectFactory(obj_factory),
   mLocationService(loc_service),
   mServerMap(server_map),
   mProximity(prox),
   mNetwork(net),
   mSendQueue(sq)
{
    // setup object which are initially residing on this server
    for(ObjectFactory::iterator it = mObjectFactory->begin(); it != mObjectFactory->end(); it++) {
        UUID obj_id = *it;
        MotionVector3f start_motion = loc_service->location(obj_id);
        Vector3f start_pos = loc_service->currentPosition(obj_id);

        mProximity->addObject(obj_id, start_motion);

        if (mServerMap->lookup(start_pos) == mID) {
            // Instantiate object
            Object* obj = mObjectFactory->object(obj_id, this);
            mObjects[obj_id] = obj;
            // Register proximity query
            mProximity->addQuery(obj_id, 100.f); // FIXME how to set proximity radius?
        }
    }
}

const ServerID& Server::id() const {
    return mID;
}

void Server::route(ObjectToObjectMessage* msg) {
    assert(msg != NULL);

    UUID src_uuid = msg->sourceObject();
    UUID dest_uuid = msg->destObject();
    ServerID destServerID=mServerMap->lookup(dest_uuid);
    ServerMessageHeader server_header(
        this->id(),
        destServerID
    );

    uint32 offset = 0;
    Network::Chunk msg_serialized;
    offset=server_header.serialize(msg_serialized, offset);
    offset = msg->serialize(msg_serialized, offset);
    if (destServerID==id()) {
        mSelfMessages.push_back(msg_serialized);
    }else {
        bool failed=!mSendQueue->addMessage(destServerID,msg_serialized,src_uuid);   
        assert(!failed);
    }
    delete msg;
}

void Server::route(Message* msg, const ServerID& dest_server) {
    assert(msg != NULL);

    ServerMessageHeader server_header(this->id(), dest_server);

    uint32 offset = 0;
    Network::Chunk msg_serialized;
    offset=server_header.serialize(msg_serialized, offset);
    offset = msg->serialize(msg_serialized, offset);
    if (dest_server==id()) {
        mSelfMessages.push_back(msg_serialized);
    }else {
        bool failed=!mSendQueue->addMessage(dest_server,msg_serialized);
        assert(!failed);
    }
    delete msg;
}

void Server::route(Message* msg, const UUID& dest_obj) {
    route(msg, mServerMap->lookup(dest_obj));
}

void Server::deliver(Message* msg) {
    switch(msg->type()) {
      case MESSAGE_TYPE_PROXIMITY:
          {
              ProximityMessage* prox_msg = static_cast<ProximityMessage*>(msg);
              Object* dest_obj = object(prox_msg->destObject());
              if (dest_obj == NULL)
                  forward(prox_msg, prox_msg->destObject());
              else
                  dest_obj->proximityMessage(prox_msg);
          }
          break;
      case MESSAGE_TYPE_LOCATION:
          {
              LocationMessage* loc_msg = static_cast<LocationMessage*>(msg);
              Object* dest_obj = object(loc_msg->destObject());
              if (dest_obj == NULL)
                  forward(loc_msg, loc_msg->destObject());
              else
                  dest_obj->locationMessage(loc_msg);
          }
          break;
      case MESSAGE_TYPE_SUBSCRIPTION:
          {
              SubscriptionMessage* subs_msg = static_cast<SubscriptionMessage*>(msg);
              Object* dest_obj = object(subs_msg->destObject());
              if (dest_obj == NULL)
                  forward(subs_msg, subs_msg->destObject());
              else
                  dest_obj->subscriptionMessage(subs_msg);
          }
        break;
      case MESSAGE_TYPE_MIGRATE:
          {
              MigrateMessage* migrate_msg = static_cast<MigrateMessage*>(msg);
              // FIXME
              delete migrate_msg;
          }
          break;
      default:
#if NDEBUG
        assert(false);
#endif
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
    route(msg, dest);
}

void Server::processChunk(const Network::Chunk&chunk) {
    Message* result;
    unsigned int offset=0;
    do {
        ServerMessageHeader hdr=ServerMessageHeader::deserialize(chunk,offset);
        offset=Message::deserialize(chunk,offset,&result);
        deliver(result);
    }while (offset<chunk.size());
}
void Server::networkTick(const Time&t) {
    mSendQueue->service();
    while (!mSelfMessages.empty()) {
        processChunk(mSelfMessages.front());
        mSelfMessages.pop_front();
    }
    Sirikata::Network::Chunk *c=NULL;
    while((c=mNetwork->receiveOne())) {
        processChunk(*c);
        delete c;
    }
}
void Server::tick(const Time& t) {
    // Update object locations
    mLocationService->tick(t);

    // Check proximity updates
    proximityTick(t);
    networkTick(t);
    // Check for object migrations
    checkObjectMigrations();
}

void Server::proximityTick(const Time& t) {
    // Check for proximity updates
    std::queue<ProximityEvent> proximity_events;
    mProximity->evaluate(t, proximity_events);

    while(!proximity_events.empty()) {
        ProximityEvent& evt = proximity_events.front();
        ProximityMessage* msg =
            new ProximityMessage(
                evt.query(),
                evt.object(),
                (evt.type() == ProximityEvent::Entered) ? ProximityMessage::Entered : ProximityMessage::Exited
            );
        route(msg, evt.query());
        proximity_events.pop();
    }
}

void Server::checkObjectMigrations() {
    // * check for objects crossing server boundaries
    //   * wrap up state and send message to other server
    //     to reinstantiate the object there
    //   * delete object on this side
}


} // namespace CBR

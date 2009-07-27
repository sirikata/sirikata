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
#include "LoadMonitor.hpp"
#include "ForwarderUtilityClasses.hpp"
#include "Forwarder.hpp"

#include "ObjectSegmentation.hpp"

#include "Random.hpp"

#include <iostream>
#include <iomanip>



namespace CBR
{

Server::Server(ServerID id, Forwarder* forwarder, ObjectFactory* obj_factory, LocationService* loc_service, CoordinateSegmentation* cseg, Proximity* prox, ObjectMessageQueue* omq, ServerMessageQueue* smq, LoadMonitor* lm, Trace* trace,ObjectSegmentation* oseg)
    : mID(id),
      mLocationService(loc_service),
      mCSeg(cseg),
      mProximity(prox),
      mCurrentTime(0),
      mTrace(trace),
      mOSeg(oseg),
      mForwarder(forwarder)
  {
    // setup object which are initially residing on this server
    for(ObjectFactory::iterator it = obj_factory->begin(); it != obj_factory->end(); it++)
    {
      UUID obj_id = *it;
      Vector3f start_pos = loc_service->currentPosition(obj_id);

      if (lookup(start_pos) == mID)
      {
          // Add object as local object to LocationService
          mLocationService->addLocalObject(obj_id);
        // Instantiate object
        Object* obj = obj_factory->object(obj_id, this->id());
        mObjects[obj_id] = obj;
        // Register proximity query
        mProximity->addQuery(obj_id, obj_factory->getProximityRadius(obj_id)); // FIXME how to set proximity radius?
      }
    }
    mForwarder->initialize(trace,cseg,oseg,loc_service,obj_factory,omq,smq,lm,&mObjects,&mCurrentTime);    //run initialization for forwarder

  }

Server::~Server()
{
    for(ObjectMap::iterator it = mObjects.begin(); it != mObjects.end(); it++) {
        UUID obj_id = it->first;
        mLocationService->removeLocalObject(obj_id);
    }
    mObjects.clear();
}

const ServerID& Server::id() const {
    return mID;
}

void Server::networkTick(const Time&t)
{
  mForwarder->tick(t);
}


void Server::tick(const Time& t)
{
  mCurrentTime = t;

  // Update object locations
  mLocationService->tick(t);


  // Check proximity updates
  proximityTick(t);
  networkTick(t);
  // Check for object migrations
  checkObjectMigrations();

  // Give objects a chance to process
  for(ObjectMap::iterator it = mObjects.begin(); it != mObjects.end(); it++)
  {
    Object* obj = it->second;
    obj->tick(t);
  }
}

void Server::proximityTick(const Time& t)
{
    // If we have global introduction, then we can just ignore proximity evaluation.
    if (GetOption(OBJECT_GLOBAL)->as<bool>() == true)
        return;

  // Check for proximity updates
  std::queue<ProximityEventInfo> proximity_events;
  mProximity->evaluate(t, proximity_events);

  OriginID origin;
  origin.id = (uint32)id();

  while(!proximity_events.empty())
  {
    ProximityEventInfo& evt = proximity_events.front();
    ProximityMessage* msg =
      new ProximityMessage(
                           origin,
                           evt.query(),
                           evt.object(),
                           (evt.type() == ProximityEventInfo::Entered) ? ProximityMessage::Entered : ProximityMessage::Exited,
                           evt.location()
                           );

    mForwarder->route(msg, evt.query());

    proximity_events.pop();
  }
}

void Server::checkObjectMigrations()
{
    // * check for objects crossing server boundaries
    // * wrap up state and send message to other server
    //     to reinstantiate the object there
    // * delete object on this side

    std::vector<UUID> migrated_objects;
    for(ObjectMap::iterator it = mObjects.begin(); it != mObjects.end(); it++)
    {
        Object* obj = it->second;
        const UUID& obj_id = obj->uuid();

        Vector3f obj_pos = mLocationService->currentPosition(obj_id);
	ServerID new_server_id = lookup(obj_pos);

        if (new_server_id != mID)
        {
          //bftm
          //          mOSeg->migrateObject(obj_id,new_server_id);
          //

            // Send out the migrate message
	    MigrateMessage* migrate_msg = wrapObjectStateForMigration(obj);
/*
	    printf("migrating object %s due to position %s \n",
		   obj_id.readableHexData().c_str(),
		   obj_pos.toString().c_str());
*/
            mForwarder->route( migrate_msg , new_server_id);

            // Stop tracking the object locally
            mLocationService->removeLocalObject(obj_id);

	    migrated_objects.push_back(obj_id);
        }
    }

    for (std::vector<UUID>::iterator it = migrated_objects.begin(); it != migrated_objects.end(); it++){
      mObjects.erase(*it);
    }
}

MigrateMessage* Server::wrapObjectStateForMigration(Object* obj)
{
  const UUID& obj_id = obj->uuid();

  OriginID origin;
  origin.id = (uint32)id();

  MigrateMessage* migrate_msg = new MigrateMessage(origin,
                                                   obj_id,
                                                   obj->proximityRadius(),
                                                   obj->subscriberSet().size(),
                                                   this->id());
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

ServerID Server::lookup(const Vector3f& pos)
{
  ServerID sid = mCSeg->lookup(pos);
  return sid;
}

ServerID Server::lookup(const UUID& obj_id)
{
  Vector3f pos = mLocationService->currentPosition(obj_id);
  ServerID sid = mCSeg->lookup(pos);
  return sid;
}

} // namespace CBR

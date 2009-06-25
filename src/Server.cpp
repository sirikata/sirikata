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

#include "ForwarderUtilityClasses.hpp" //bftm
#include "Forwarder.hpp" //bftm

namespace CBR
{
  
  Server::Server(ServerID id, ObjectFactory* obj_factory, LocationService* loc_service, CoordinateSegmentation* cseg, Proximity* prox, ObjectMessageQueue* omq, ServerMessageQueue* smq, LoadMonitor* lm, Trace* trace)
    : mID(id),
      mObjectFactory(obj_factory),
      mLocationService(loc_service),
      mCSeg(cseg),
      mProximity(prox),
      //   mObjectMessageQueue(omq), //bftm
      //   mServerMessageQueue(smq), //bftm
      //   mLoadMonitor(lm),         //bftm
      mCurrentTime(0),
      mTrace(trace),
      mForwarder(trace,cseg,loc_service,obj_factory,omq,smq,lm,id)
  {
    // setup object which are initially residing on this server
    for(ObjectFactory::iterator it = mObjectFactory->begin(); it != mObjectFactory->end(); it++)
    {
      UUID obj_id = *it;
      Vector3f start_pos = loc_service->currentPosition(obj_id);

      if (lookup(start_pos) == mID)
      {
        // Instantiate object
        Object* obj = mObjectFactory->object(obj_id, this->id());
        mObjects[obj_id] = obj;
        // Register proximity query
        mProximity->addQuery(obj_id, mObjectFactory->getProximityRadius(obj_id)); // FIXME how to set proximity radius?
      }
    }

    //bftm
    //run initialization for forwarder
    mForwarder.initialize(&mObjects,&mCurrentTime);

  }

Server::~Server()
{
}

const ServerID& Server::id() const {
    return mID;
}

void Server::networkTick(const Time&t)
{
  //        void tick(const Time&t,int servID);
  mForwarder.tick(t,this->id()); //call mForwarder to perform networkTick
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
    //bftm    route(msg, evt.query());
    mForwarder.route(msg, evt.query());

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

        if (new_server_id != mID) {
	    MigrateMessage* migrate_msg = wrapObjectStateForMigration(obj);
/*
	    printf("migrating object %s due to position %s \n",
		   obj_id.readableHexData().c_str(),
		   obj_pos.toString().c_str());
*/
            
            //bftm  	    route( migrate_msg , new_server_id);
            mForwarder.route( migrate_msg , new_server_id);
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

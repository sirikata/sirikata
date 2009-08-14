#include "Network.hpp"
#include "Server.hpp"
#include "Proximity.hpp"
#include "Object.hpp"
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

#include "ObjectConnection.hpp"

#include "Random.hpp"

#include <iostream>
#include <iomanip>



namespace CBR
{

Server::Server(ServerID id, Forwarder* forwarder, LocationService* loc_service, CoordinateSegmentation* cseg, Proximity* prox, ObjectMessageQueue* omq, ServerMessageQueue* smq, LoadMonitor* lm, Trace* trace,ObjectSegmentation* oseg)
    : mID(id),
      mLocationService(loc_service),
      mCSeg(cseg),
      mProximity(prox),
      mCurrentTime(Time::null()),
      mTrace(trace),
      mOSeg(oseg),
      mForwarder(forwarder)
  {
      mForwarder->registerMessageRecipient(MESSAGE_TYPE_MIGRATE, this);

    mForwarder->initialize(trace,cseg,oseg,loc_service,omq,smq,lm,&mCurrentTime, mProximity);    //run initialization for forwarder

  }

Server::~Server()
{
    mForwarder->unregisterMessageRecipient(MESSAGE_TYPE_MIGRATE, this);

    for(ObjectMap::iterator it = mObjects.begin(); it != mObjects.end(); it++) {
        UUID obj_id = it->first;
        mLocationService->removeLocalObject(obj_id);
        // FIXME there's probably quite a bit more cleanup to do here
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


void Server::connect(Object* obj, bool wait_for_migrate) {
    if (!wait_for_migrate) {
        UUID obj_id = obj->uuid();
        mObjects[obj_id] = obj;
        // Add object as local object to LocationService
        mLocationService->addLocalObject(obj_id);
        // Register proximity query
        mProximity->addQuery(obj_id, obj->queryAngle()); // FIXME how to set proximity radius?
        // Create "connection" and let forwarder deliver to it
        ObjectConnection* conn = new ObjectConnection(obj, mTrace);
        mForwarder->addObjectConnection(obj_id, conn);
    }
    else {
        mObjectsAwaitingMigration[obj->uuid()] = obj;
    }
}

void Server::receiveMessage(Message* msg) {
    MigrateMessage* migrate_msg = dynamic_cast<MigrateMessage*>(msg);
    if (migrate_msg != NULL) {
        const UUID obj_id = migrate_msg->object();

        ObjectMap::iterator obj_map_it = mObjectsAwaitingMigration.find(obj_id);
        if (obj_map_it == mObjectsAwaitingMigration.end()) {
            printf("Warning: Got migration message, but this server doesn't have a connection for that object.\n");
        }

        Object* obj = obj_map_it->second;

        obj->migrateMessage(migrate_msg);

        // Move from list waiting for migration message to active objects
        mObjectsAwaitingMigration.erase(obj_map_it);
        mObjects[obj_id] = obj;

        ServerID idOSegAckTo = migrate_msg->messageFrom();

        // Update LOC to indicate we have this object locally
        mLocationService->addLocalObject(obj_id);

        //update our oseg to show that we know that we have this object now.
        mOSeg->addObject(obj_id, mID);

        //We also send an oseg message to the server that the object was formerly hosted on.  This is an acknwoledge message that says, we're handling the object now...that's going to be the server with the origin tag affixed.
        Message* oseg_ack_msg;
        //              mOSeg->generateAcknowledgeMessage(obj, idOSegAckTo,oseg_ack_msg);
        oseg_ack_msg = mOSeg->generateAcknowledgeMessage(obj, idOSegAckTo);

        if (oseg_ack_msg != NULL)
        {
            mForwarder->route(oseg_ack_msg, (dynamic_cast <OSegMigrateMessage*>(oseg_ack_msg))->getMessageDestination(),false);
        }

        // Finally, subscribe the object for proximity queries
        mProximity->addQuery(obj_id, obj->queryAngle());

        // Create "connection" and let forwarder deliver to it
        ObjectConnection* conn = new ObjectConnection(obj, mTrace);
        mForwarder->addObjectConnection(obj_id, conn);

        delete migrate_msg;
    }
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

  while(!proximity_events.empty())
  {
    ProximityEventInfo& evt = proximity_events.front();
    CBR::Protocol::Prox::ProximityResults prox_results;

    if (evt.type() == ProximityEventInfo::Entered) {
        CBR::Protocol::Prox::IObjectAddition addition = prox_results.add_addition();
        addition.set_object( evt.object() );

        CBR::Protocol::Prox::ITimedMotionVector motion = addition.mutable_location();
        TimedMotionVector3f loc = mLocationService->location(evt.object());
        motion.set_t(loc.updateTime());
        motion.set_position(loc.position());
        motion.set_velocity(loc.velocity());

        addition.set_bounds( mLocationService->bounds(evt.object()) );
    }
    else {
        CBR::Protocol::Prox::IObjectRemoval removal = prox_results.add_removal();
        removal.set_object( evt.object() );
    }

    CBR::Protocol::Object::ObjectMessage* obj_msg = new CBR::Protocol::Object::ObjectMessage();
    obj_msg->set_source_object(UUID::null());
    obj_msg->set_source_port(OBJECT_PORT_PROXIMITY);
    obj_msg->set_dest_object(evt.query());
    obj_msg->set_dest_port(OBJECT_PORT_PROXIMITY);
    obj_msg->set_unique(GenerateUniqueID(id()));
    obj_msg->set_payload( serializePBJMessage(prox_results) );

    mForwarder->route(obj_msg);

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

            // Stop any proximity queries for this object
            mProximity->removeQuery(obj_id);

            // Stop tracking the object locally
            mLocationService->removeLocalObject(obj_id);

            // Stop Forwarder from delivering via this Object's
            // connection, destroy said connection
            ObjectConnection* migrated_conn = mForwarder->removeObjectConnection(obj_id);
            delete migrated_conn;

	    migrated_objects.push_back(obj_id);
        }
    }

    for (std::vector<UUID>::iterator it = migrated_objects.begin(); it != migrated_objects.end(); it++){
        ObjectMap::iterator obj_map_it = mObjects.find(*it);
        Object* obj = obj_map_it->second;
        delete obj;
        mObjects.erase(obj_map_it);
    }
}

MigrateMessage* Server::wrapObjectStateForMigration(Object* obj)
{
  const UUID& obj_id = obj->uuid();

  MigrateMessage* migrate_msg = new MigrateMessage(
      id(),
      obj_id,
      obj->queryAngle(),
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

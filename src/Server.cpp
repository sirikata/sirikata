#include "Network.hpp"
#include "Server.hpp"
#include "Proximity.hpp"
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
      mForwarder->registerObjectMessageRecipient(OBJECT_PORT_SESSION, this);

    mForwarder->initialize(trace,cseg,oseg,loc_service,omq,smq,lm,&mCurrentTime, mProximity);    //run initialization for forwarder

  }

Server::~Server()
{
    mForwarder->unregisterObjectMessageRecipient(OBJECT_PORT_SESSION, this);
    mForwarder->unregisterMessageRecipient(MESSAGE_TYPE_MIGRATE, this);

    for(ObjectConnectionMap::iterator it = mObjects.begin(); it != mObjects.end(); it++) {
        UUID obj_id = it->first;

        // Stop any proximity queries for this object
        mProximity->removeQuery(obj_id);

        mLocationService->removeLocalObject(obj_id);

        // Stop Forwarder from delivering via this Object's
        // connection, destroy said connection
        ObjectConnection* migrated_conn = mForwarder->removeObjectConnection(obj_id);
        mClosingConnections.insert(migrated_conn);

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

  // Tick closing object connections, deleting them when they are
  ObjectConnectionSet persistingConnections;
  for(ObjectConnectionSet::iterator it = mClosingConnections.begin(); it != mClosingConnections.end(); it++) {
      ObjectConnection* conn = *it;
      conn->tick(t);
      if (conn->empty())
          delete conn;
      else
          persistingConnections.insert(conn);
  }
  mClosingConnections.swap(persistingConnections);
}

void Server::handleOpenConnection(ObjectConnection* conn) {
    mConnectingObjects[conn->id()] = conn;
}

bool Server::receiveObjectHostMessage(const std::string& msg) {
    // All messages from object host to space server are object messages
    // They are either to another object or to the space (UUID 0)
    CBR::Protocol::Object::ObjectMessage* obj_msg = new CBR::Protocol::Object::ObjectMessage();
    bool parse_success = obj_msg->ParseFromArray((void*)&msg[0], msg.size());
    assert(parse_success);

    // The object could be migrating and we get outdated packets.  Currently this can
    // happen because we need to maintain the connection long enough to deliver the init migration
    // message.  Currently we just discard these messages, but we may need to account for this.
    // NOTE that we check connecting objects as well since we need to get past this point to deliver
    // Session::Connect messages.
    if (mObjects.find(obj_msg->source_object()) == mObjects.end() &&
        mConnectingObjects.find(obj_msg->source_object()) == mConnectingObjects.end())
    {
        //printf("Warning: Server got message from object after migration started.\n");
        return true;
    }

    return mForwarder->routeObjectHostMessage(obj_msg);
}

// Handle Session messages from an object
void Server::handleSessionMessage(ObjectConnection* conn, const std::string& session_payload) {
    CBR::Protocol::Session::Container session_msg;
    bool parse_success = session_msg.ParseFromArray((void*)&session_payload[0], session_payload.size());
    assert(parse_success);

    // Connect messages
    if (session_msg.has_connect())
        handleConnect(conn, session_msg.connect());

    // InitiateMigration messages
    assert(!session_msg.has_init_migration());

    // Migrate messages
    if (session_msg.has_migrate())
        handleMigrate(conn, session_msg.migrate());
}

// Handle Connect message from object
void Server::handleConnect(ObjectConnection* conn, const CBR::Protocol::Session::Connect& connect_msg) {
    UUID obj_id = conn->id();
    assert(obj_id == connect_msg.object());

    mObjects[obj_id] = conn;

    // If the connection is in the connecting list, remove it
    mConnectingObjects.erase(obj_id);

    // Add object as local object to LocationService
    TimedMotionVector3f loc( connect_msg.loc().t(), MotionVector3f(connect_msg.loc().position(), connect_msg.loc().velocity()) );
    mLocationService->addLocalObject(obj_id, loc, connect_msg.bounds());
    // Register proximity query
    mProximity->addQuery(obj_id, SolidAngle(connect_msg.query_angle()));
    // Allow the forwarder to send to ship messages to this connection
    mForwarder->addObjectConnection(obj_id, conn);
}

// Handle Migrate message from object
void Server::handleMigrate(ObjectConnection* conn, const CBR::Protocol::Session::Migrate& migrate_msg) {
    assert(conn->id() == migrate_msg.object());
    mObjectsAwaitingMigration[migrate_msg.object()] = conn;

    // If the connection is in the connecting list, remove it
    mConnectingObjects.erase(migrate_msg.object());

    // Try to handle this migration if all info is available
    handleMigration(migrate_msg.object());
}

void Server::receiveMessage(Message* msg) {
    MigrateMessage* migrate_msg = dynamic_cast<MigrateMessage*>(msg);
    if (migrate_msg != NULL) {
        const UUID obj_id = migrate_msg->contents.object();
        mObjectMigrations[obj_id] = new CBR::Protocol::Migration::MigrationMessage( migrate_msg->contents );
        // Try to handle this migration if all the info is available
        handleMigration(obj_id);
    }

    delete msg;
}

void Server::receiveMessage(const CBR::Protocol::Object::ObjectMessage& msg) {
    switch( msg.dest_port() ) {
      case OBJECT_PORT_SESSION:
          {
              ObjectConnection* conn = NULL;

              {
                  ObjectConnectionMap::iterator it = mObjects.find(msg.source_object());
                  if(it != mObjects.end())
                      conn = it->second;
              }

              {
                  ObjectConnectionMap::iterator it = mConnectingObjects.find(msg.source_object());
                  if (it != mObjects.end())
                      conn = it->second;
              }

              handleSessionMessage(conn, msg.payload());
          }
        break;
      default:
        printf("Received message for unknown port: %d\n", msg.dest_port());
        break;
    }
}

void Server::handleMigration(const UUID& obj_id) {
    // Try to find the info in both lists -- the connection and migration information
    ObjectConnectionMap::iterator obj_map_it = mObjectsAwaitingMigration.find(obj_id);
    if (obj_map_it == mObjectsAwaitingMigration.end())
        return;

    ObjectMigrationMap::iterator migration_map_it = mObjectMigrations.find(obj_id);
    if (migration_map_it == mObjectMigrations.end())
        return;

    // Get the data from the two maps
    ObjectConnection* obj_conn = obj_map_it->second;
    CBR::Protocol::Migration::MigrationMessage* migrate_msg = migration_map_it->second;

    // Extract the migration message data
    TimedMotionVector3f obj_loc(
        migrate_msg->loc().t(),
        MotionVector3f( migrate_msg->loc().position(), migrate_msg->loc().velocity() )
    );
    BoundingSphere3f obj_bounds( migrate_msg->bounds() );
    SolidAngle obj_query_angle( migrate_msg->query_angle() );
    std::vector<UUID> obj_subs;
    for(uint32 i = 0; i < migrate_msg->subscriber_size(); i++)
        obj_subs.push_back(migrate_msg->subscriber(i));


    obj_conn->handleMigrateMessage(obj_id, obj_query_angle, obj_subs);

    // Move from list waiting for migration message to active objects
    mObjects[obj_id] = obj_conn;


    // Update LOC to indicate we have this object locally
    mLocationService->addLocalObject(obj_id, obj_loc, obj_bounds);

    //update our oseg to show that we know that we have this object now.
    mOSeg->addObject(obj_id, mID);

    //We also send an oseg message to the server that the object was formerly hosted on.  This is an acknwoledge message that says, we're handling the object now...that's going to be the server with the origin tag affixed.
    ServerID idOSegAckTo = (ServerID)migrate_msg->source_server();
    Message* oseg_ack_msg;
    //              mOSeg->generateAcknowledgeMessage(obj_id, idOSegAckTo,oseg_ack_msg);
    oseg_ack_msg = mOSeg->generateAcknowledgeMessage(obj_id, idOSegAckTo);

    if (oseg_ack_msg != NULL)
        mForwarder->route(oseg_ack_msg, (dynamic_cast <OSegMigrateMessage*>(oseg_ack_msg))->getMessageDestination(),false);

    // Finally, subscribe the object for proximity queries
    mProximity->addQuery(obj_id, obj_query_angle);

    // Allow the forwarder to deliver to this connection
    mForwarder->addObjectConnection(obj_id, obj_conn);


    // Clean out the two records from the migration maps
    mObjectsAwaitingMigration.erase(obj_map_it);
    mObjectMigrations.erase(migration_map_it);
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
    for(ObjectConnectionMap::iterator it = mObjects.begin(); it != mObjects.end(); it++)
    {
        const UUID& obj_id = it->first;
        ObjectConnection* obj_conn = it->second;

        Vector3f obj_pos = mLocationService->currentPosition(obj_id);
	ServerID new_server_id = lookup(obj_pos);

        if (new_server_id != mID)
        {
            // FIXME While we're working on the transition to a separate object host
            // we do 2 things when we detect a server boundary crossing:
            // 1. Generate a message which is sent to the object host telling it to
            //    transition to the new server
            // 2. Generate the migrate message which contains the current state of
            //    the object which will be reconstituted on the other space server.


            // Part 1
            CBR::Protocol::Session::Container session_msg;
            CBR::Protocol::Session::IInitiateMigration init_migration_msg = session_msg.mutable_init_migration();
            init_migration_msg.set_new_server( (uint64)new_server_id );
            CBR::Protocol::Object::ObjectMessage* init_migr_obj_msg = new CBR::Protocol::Object::ObjectMessage();
            init_migr_obj_msg->set_source_object(UUID::null());
            init_migr_obj_msg->set_source_port(OBJECT_PORT_SESSION);
            init_migr_obj_msg->set_dest_object(obj_id);
            init_migr_obj_msg->set_dest_port(OBJECT_PORT_SESSION);
            init_migr_obj_msg->set_unique(GenerateUniqueID(id()));
            init_migr_obj_msg->set_payload( serializePBJMessage(session_msg) );
            obj_conn->deliver(*init_migr_obj_msg); // Note that we can't go through the forwarder since it will stop delivering to this object connection right after this
            delete init_migr_obj_msg;
            //printf("Routed init migration msg to %s\n", obj_id.toString().c_str());

            // Part 2

          //bftm
          //          mOSeg->migrateObject(obj_id,new_server_id);
          //

            // Send out the migrate message
            MigrateMessage* migrate_msg = new MigrateMessage(id());
            migrate_msg->contents.set_source_server(this->id());
            obj_conn->fillMigrateMessage(migrate_msg);
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
            mClosingConnections.insert(migrated_conn);

	    migrated_objects.push_back(obj_id);
        }
    }

    for (std::vector<UUID>::iterator it = migrated_objects.begin(); it != migrated_objects.end(); it++){
        ObjectConnectionMap::iterator obj_map_it = mObjects.find(*it);
        mObjects.erase(obj_map_it);
    }
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

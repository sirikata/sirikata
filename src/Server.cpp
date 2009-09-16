#include "Network.hpp"
#include "Server.hpp"
#include "Proximity.hpp"
#include "CoordinateSegmentation.hpp"
#include "Message.hpp"
#include "Statistics.hpp"
#include "Options.hpp"
#include "LoadMonitor.hpp"
#include "Forwarder.hpp"
#include "MigrationMonitor.hpp"

#include "ObjectSegmentation.hpp"

#include "ObjectConnection.hpp"
#include "ObjectHostConnectionManager.hpp"

#include "Random.hpp"

#include <iostream>
#include <iomanip>



namespace CBR
{

Server::Server(SpaceContext* ctx, Forwarder* forwarder, LocationService* loc_service, CoordinateSegmentation* cseg, Proximity* prox, LoadMonitor* lm, ObjectSegmentation* oseg, Address4* oh_listen_addr)
 : mContext(ctx),
   mLocationService(loc_service),
   mCSeg(cseg),
   mProximity(prox),
   mOSeg(oseg),
   mForwarder(forwarder),
   mMigrationMonitor(),
   mLoadMonitor(lm),
   mObjectHostConnectionManager(NULL),
   mProfiler("Server Loop")
{
      mForwarder->registerMessageRecipient(MESSAGE_TYPE_MIGRATE, this);
      mForwarder->registerMessageRecipient(MESSAGE_TYPE_KILL_OBJ_CONN, this);

      mMigrationMonitor = new MigrationMonitor(mLocationService, mCSeg);

    mObjectHostConnectionManager = new ObjectHostConnectionManager(
        mContext, *oh_listen_addr,
        std::tr1::bind(&Server::handleObjectHostMessage, this, std::tr1::placeholders::_1, std::tr1::placeholders::_2)
    );

    mProfiler.addStage("Loc");
    mProfiler.addStage("Prox");
    mProfiler.addStage("Forwarder");
    mProfiler.addStage("Load Monitor");
    mProfiler.addStage("Object Hosts");
    mProfiler.addStage("Migration Monitor");
}

Server::~Server()
{
    if (GetOption(PROFILE)->as<bool>())
        mProfiler.report();

    delete mObjectHostConnectionManager;

    mForwarder->unregisterMessageRecipient(MESSAGE_TYPE_MIGRATE, this);
    mForwarder->unregisterMessageRecipient(MESSAGE_TYPE_KILL_OBJ_CONN, this);

    printf("mObjects.size=%d\n", (uint32)mObjects.size());

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

ObjectConnection* Server::getObjectConnection(const UUID& object_id) const {
    ObjectConnectionMap::const_iterator it = mObjects.find(object_id);
    if(it != mObjects.end())
        return it->second;

    // XXX FIXME migrating objects?

    return NULL;
}

void Server::serviceObjectHostNetwork() {
    mObjectHostConnectionManager->service();

  // Tick all active connections
  for(ObjectConnectionMap::iterator it = mObjects.begin(); it != mObjects.end(); it++) {
      ObjectConnection* conn = it->second;
      conn->service();
  }


  //bftm add
  for (ObjectConnectionMap::iterator it = mMigratingConnections.begin(); it != mMigratingConnections.end(); ++it)
  {
    ObjectConnection* conn = it->second;
    conn->service();
  }


  // Tick closing object connections, deleting them when they are
  ObjectConnectionSet persistingConnections;
  for(ObjectConnectionSet::iterator it = mClosingConnections.begin(); it != mClosingConnections.end(); it++) {
      ObjectConnection* conn = *it;
      conn->service();
      if (conn->empty())
          delete conn;
      else
          persistingConnections.insert(conn);
  }
  mClosingConnections.swap(persistingConnections);
}

void Server::handleObjectHostMessage(const ObjectHostConnectionManager::ConnectionID& conn_id, CBR::Protocol::Object::ObjectMessage* obj_msg) {
    // Before admitting a message, we need to do some sanity checks.  Also, some types of messages get
    // exceptions for bootstrapping purposes (namely session messages to the space).

    // 1. If the source is the space, somebody is messing with us.
    bool space_source = (obj_msg->source_object() == UUID::null());
    if (space_source) {
        SILOG(cbr,error,"Got message from object host claiming to be from space.");
        delete obj_msg;
        return;
    }

    // 2. For connection bootstrapping purposes we need to exempt session messages destined for the space.
    // Note that we need to check this before the connected sanity check since obviously the object won't
    // be connected yet.  We dispatch directly from here since this needs information about the object host
    // connection to be passed along as well.
    bool space_dest = (obj_msg->dest_object() == UUID::null());
    bool session_msg = (obj_msg->dest_port() == OBJECT_PORT_SESSION);
    if (space_dest && session_msg)
    {
        handleSessionMessage(conn_id, *obj_msg);
        delete obj_msg;
        return;
    }


    // 3. If we don't have a connection for the source object, we can't do anything with it.
    // The object could be migrating and we get outdated packets.  Currently this can
    // happen because we need to maintain the connection long enough to deliver the init migration
    // message.  Currently we just discard these messages, but we may need to account for this.
    // NOTE that we check connecting objects as well since we need to get past this point to deliver
    // Session messages.
    bool source_connected = mObjects.find(obj_msg->source_object()) != mObjects.end(); // FIXME migrating objects?
    if (!source_connected)
    {
        if (mObjectsAwaitingMigration.find(obj_msg->source_object()) == mObjectsAwaitingMigration.end() &&
            mObjectMigrations.find(obj_msg->source_object()) == mObjectMigrations.end())
        {
            SILOG(cbr,warn,"Got message for unknown object: " << obj_msg->source_object().toString());
        }
        else
        {
            SILOG(cbr,warn,"Server got message from object after migration started: " << obj_msg->source_object().toString());
        }

        delete obj_msg;

        return;
    }

    // 4. Finally, if we've passed all these tests, then everything looks good and we can route it
    bool route_success = mForwarder->routeObjectHostMessage(obj_msg);
    // FIXME handle forwarding failure
}

// Handle Session messages from an object
void Server::handleSessionMessage(const ObjectHostConnectionManager::ConnectionID& oh_conn_id, const CBR::Protocol::Object::ObjectMessage& msg) {
    CBR::Protocol::Session::Container session_msg;
    bool parse_success = session_msg.ParseFromString(msg.payload());
    assert(parse_success);


    // Connect or migrate messages
    if (session_msg.has_connect()) {
        if (session_msg.connect().type() == CBR::Protocol::Session::Connect::Fresh)
            handleConnect(oh_conn_id, msg, session_msg.connect());
        else if (session_msg.connect().type() == CBR::Protocol::Session::Connect::Migration)
        {
            handleMigrate(oh_conn_id, msg, session_msg.connect());
        }
        else
            SILOG(space,error,"Unknown connection message type");
    }

    // InitiateMigration messages
    assert(!session_msg.has_init_migration());

}

// Handle Connect message from object
void Server::handleConnect(const ObjectHostConnectionManager::ConnectionID& oh_conn_id, const CBR::Protocol::Object::ObjectMessage& container, const CBR::Protocol::Session::Connect& connect_msg) {
    UUID obj_id = container.source_object();
    assert( getObjectConnection(obj_id) == NULL );

    // If the requested location isn't on this server, redirect
    TimedMotionVector3f loc( connect_msg.loc().t(), MotionVector3f(connect_msg.loc().position(), connect_msg.loc().velocity()) );
    Vector3f curpos = loc.extrapolate(mContext->time).position();
    bool in_server_region = mMigrationMonitor->onThisServer(curpos);
    ServerID loc_server = mCSeg->lookup(curpos);

    if(loc_server == NullServerID || (loc_server == mContext->id() && !in_server_region)) {
        // Either CSeg says no server handles the specified region or
        // that we should, but it doesn't actually land in our region
        // (i.e. things were probably clamped invalidly)

        // Create and send redirect reply
        CBR::Protocol::Session::Container response_container;
        CBR::Protocol::Session::IConnectResponse response = response_container.mutable_connect_response();
        response.set_response( CBR::Protocol::Session::ConnectResponse::Error );

        CBR::Protocol::Object::ObjectMessage* obj_response = createObjectMessage(
            mContext->id(),
            UUID::null(), OBJECT_PORT_SESSION,
            obj_id, OBJECT_PORT_SESSION,
            serializePBJMessage(response_container)
        );
        // Sent directly via object host connection manager because we don't have an ObjectConnection
        mObjectHostConnectionManager->send( oh_conn_id, obj_response );
        return;
    }

    if (loc_server != mContext->id()) {
        // Since we passed the previous test, this just means they tried to connect
        // to the wrong server => redirect

        // Create and send redirect reply
        CBR::Protocol::Session::Container response_container;
        CBR::Protocol::Session::IConnectResponse response = response_container.mutable_connect_response();
        response.set_response( CBR::Protocol::Session::ConnectResponse::Redirect );
        response.set_redirect(loc_server);

        CBR::Protocol::Object::ObjectMessage* obj_response = createObjectMessage(
            mContext->id(),
            UUID::null(), OBJECT_PORT_SESSION,
            obj_id, OBJECT_PORT_SESSION,
            serializePBJMessage(response_container)
        );
        // Sent directly via object host connection manager because we don't have an ObjectConnection
        mObjectHostConnectionManager->send( oh_conn_id, obj_response );
        return;
    }

    // FIXME sanity check the new connection
    // -- authentication
    // -- verify object may connect, i.e. not already in system (e.g. check oseg)

    // Create and store the connection
    ObjectConnection* conn = new ObjectConnection(obj_id, mObjectHostConnectionManager, oh_conn_id);
    mObjects[obj_id] = conn;

    // Add object as local object to LocationService
    mLocationService->addLocalObject(obj_id, loc, connect_msg.bounds());
    //update our oseg to show that we know that we have this object now.
    mOSeg->addObject(obj_id, mContext->id(), false); //don't need to generate an acknowledge message to myself, of course
    // Register proximity query
    if (connect_msg.has_query_angle())
        mProximity->addQuery(obj_id, SolidAngle(connect_msg.query_angle()));
    // Allow the forwarder to send to ship messages to this connection
    mForwarder->addObjectConnection(obj_id, conn);

    // Send reply back indicating that the connection was successful
    CBR::Protocol::Session::Container response_container;
    CBR::Protocol::Session::IConnectResponse response = response_container.mutable_connect_response();
    response.set_response( CBR::Protocol::Session::ConnectResponse::Success );

    CBR::Protocol::Object::ObjectMessage* obj_response = createObjectMessage(
        mContext->id(),
        UUID::null(), OBJECT_PORT_SESSION,
        obj_id, OBJECT_PORT_SESSION,
        serializePBJMessage(response_container)
    );

    conn->send( obj_response );
}

// Handle Migrate message from object
//this is called by the receiving server.
void Server::handleMigrate(const ObjectHostConnectionManager::ConnectionID& oh_conn_id, const CBR::Protocol::Object::ObjectMessage& container, const CBR::Protocol::Session::Connect& migrate_msg)
{
    UUID obj_id = container.source_object();

    assert( getObjectConnection(obj_id) == NULL );

    // FIXME sanity check the new connection
    // -- authentication
    // -- verify object may connect, i.e. not already in system (e.g. check oseg)
    // Verify the requested position is on this server

    // Create and store the connection
    ObjectConnection* conn = new ObjectConnection(obj_id, mObjectHostConnectionManager, oh_conn_id);
    mObjectsAwaitingMigration[obj_id] = conn;

    // Try to handle this migration if all info is available

    handleMigration(obj_id);

    //    handleMigration(migrate_msg.object());

}

void Server::receiveMessage(Message* msg) {
    MigrateMessage* migrate_msg = dynamic_cast<MigrateMessage*>(msg);
    if (migrate_msg != NULL) {
        const UUID obj_id = migrate_msg->contents.object();
        mObjectMigrations[obj_id] = new CBR::Protocol::Migration::MigrationMessage( migrate_msg->contents );
        // Try to handle this migration if all the info is available

        handleMigration(obj_id);
    }

    KillObjConnMessage* kill_obj_conn_msg = dynamic_cast<KillObjConnMessage*>(msg);
    if (kill_obj_conn_msg != NULL)
    {
      const UUID obj_id = kill_obj_conn_msg->contents.m_objid();
      killObjectConnection(obj_id);

    }

    delete msg;
}


//handleMigration to this server.
void Server::handleMigration(const UUID& obj_id)
{

    // Try to find the info in both lists -- the connection and migration information
    ObjectConnectionMap::iterator obj_map_it = mObjectsAwaitingMigration.find(obj_id);
    if (obj_map_it == mObjectsAwaitingMigration.end())
    {
        return;
    }

    ObjectMigrationMap::iterator migration_map_it = mObjectMigrations.find(obj_id);
    if (migration_map_it == mObjectMigrations.end())
    {
        return;
    }


    // Get the data from the two maps
    ObjectConnection* obj_conn = obj_map_it->second;
    CBR::Protocol::Migration::MigrationMessage* migrate_msg = migration_map_it->second;


    // Extract the migration message data
    TimedMotionVector3f obj_loc(
        migrate_msg->loc().t(),
        MotionVector3f( migrate_msg->loc().position(), migrate_msg->loc().velocity() )
    );
    BoundingSphere3f obj_bounds( migrate_msg->bounds() );


    // Move from list waiting for migration message to active objects
    mObjects[obj_id] = obj_conn;


    // Update LOC to indicate we have this object locally
    mLocationService->addLocalObject(obj_id, obj_loc, obj_bounds);

    //update our oseg to show that we know that we have this object now.
    ServerID idOSegAckTo = (ServerID)migrate_msg->source_server();
    mOSeg->addObject(obj_id, idOSegAckTo, true);//true states to send an ack message to idOSegAckTo

    // Handle any data packed into the migration message for space components
    for(int32 i = 0; i < migrate_msg->client_data_size(); i++) {
        CBR::Protocol::Migration::MigrationClientData client_data = migrate_msg->client_data(i);
        std::string tag = client_data.key();
        // FIXME these should live in a map, how do we deal with ordering constraints?
        if (tag == "prox") {
            assert( tag == mProximity->migrationClientTag() );
            mProximity->receiveMigrationData(obj_id, /* FIXME */NullServerID, mContext->id(), client_data.data());
        }
        else {
            SILOG(space,error,"Got unknown tag for client migration data");
        }
    }

    // Allow the forwarder to deliver to this connection
    mForwarder->addObjectConnection(obj_id, obj_conn);


    // Clean out the two records from the migration maps
    mObjectsAwaitingMigration.erase(obj_map_it);
    mObjectMigrations.erase(migration_map_it);


    // Send reply back indicating that the migration was successful
    CBR::Protocol::Session::Container response_container;
    CBR::Protocol::Session::IConnectResponse response = response_container.mutable_connect_response();
    response.set_response( CBR::Protocol::Session::ConnectResponse::Success );

    CBR::Protocol::Object::ObjectMessage* obj_response = createObjectMessage(
        mContext->id(),
        UUID::null(), OBJECT_PORT_SESSION,
        obj_id, OBJECT_PORT_SESSION,
        serializePBJMessage(response_container)
    );
    obj_conn->send( obj_response );
}

void Server::service() {
    mProfiler.startIteration();

    mLocationService->service();  mProfiler.finishedStage();
    serviceProximity();           mProfiler.finishedStage();
    mForwarder->service();        mProfiler.finishedStage();
    mLoadMonitor->service();      mProfiler.finishedStage();
    serviceObjectHostNetwork();   mProfiler.finishedStage();
    checkObjectMigrations();      mProfiler.finishedStage();
}

void Server::serviceProximity() {
    // If we have global introduction, then we can just ignore proximity evaluation.
    if (GetOption(OBJECT_GLOBAL)->as<bool>() == true)
        return;

  // Check for proximity updates
  mProximity->service();
}


//this is called by the server that is sending an object to another server.
void Server::checkObjectMigrations()
{
    // * check for objects crossing server boundaries
    // * wrap up state and send message to other server
    //     to reinstantiate the object there
    // * delete object on this side

    std::set<UUID> to_migrate = mMigrationMonitor->service();

    for(std::set<UUID>::iterator it = to_migrate.begin(); it != to_migrate.end(); it++) {
        const UUID& obj_id = *it;

        if (mOSeg->clearToMigrate(obj_id)) //needs to check whether migration to this server has finished before can begin migrating to another server.
        {
            ObjectConnection* obj_conn = mObjects[obj_id];

            Vector3f obj_pos = mLocationService->currentPosition(obj_id);
            ServerID new_server_id = mCSeg->lookup(obj_pos);

            // FIXME should be this
            //assert(new_server_id != mContext->id());
            // but I'm getting inconsistencies, so we have to just trust CSeg to have the final say
            if (new_server_id == mContext->id()) continue;


            CBR::Protocol::Session::Container session_msg;
            CBR::Protocol::Session::IInitiateMigration init_migration_msg = session_msg.mutable_init_migration();
            init_migration_msg.set_new_server( (uint64)new_server_id );
            CBR::Protocol::Object::ObjectMessage* init_migr_obj_msg = createObjectMessage(
                mContext->id(),
                UUID::null(), OBJECT_PORT_SESSION,
                obj_id, OBJECT_PORT_SESSION,
                serializePBJMessage(session_msg)
            );
            obj_conn->send(init_migr_obj_msg); // Note that we can't go through the forwarder since it will stop delivering to this object connection right after this

            mOSeg->migrateObject(obj_id,new_server_id);

            // Send out the migrate message
            MigrateMessage* migrate_msg = new MigrateMessage(mContext->id());
            migrate_msg->contents.set_source_server(mContext->id());
            migrate_msg->contents.set_object(obj_id);
            CBR::Protocol::Migration::ITimedMotionVector migrate_loc = migrate_msg->contents.mutable_loc();
            TimedMotionVector3f obj_loc = mLocationService->location(obj_id);
            migrate_loc.set_t( obj_loc.updateTime() );
            migrate_loc.set_position( obj_loc.position() );
            migrate_loc.set_velocity( obj_loc.velocity() );
            migrate_msg->contents.set_bounds( mLocationService->bounds(obj_id) );

            // FIXME we should allow components to package up state here
            // FIXME we should generate these from some map instead of directly
            std::string prox_data = mProximity->generateMigrationData(obj_id, mContext->id(), new_server_id);
            if (!prox_data.empty()) {
                CBR::Protocol::Migration::IMigrationClientData client_data = migrate_msg->contents.add_client_data();
                client_data.set_key( mProximity->migrationClientTag() );
                client_data.set_data( prox_data );
            }

            // Stop tracking the object locally
            mLocationService->removeLocalObject(obj_id);

            mForwarder->route( MessageRouter::MIGRATES, migrate_msg , new_server_id);


            // Stop Forwarder from delivering via this Object's
            // connection, destroy said connection

            //bftm: candidate for multiple obj connections
            if (mOSeg->getOSegType() == LOC_OSEG)
            {
                ObjectConnection* migrated_conn = mForwarder->removeObjectConnection(obj_id);
                mClosingConnections.insert(migrated_conn);
            }
            if(mOSeg->getOSegType() == CRAQ_OSEG)
            {
                //end bftm change
                mMigratingConnections[obj_id] = mForwarder->getObjectConnection(obj_id);
            }

            mObjects.erase(obj_id);
        }
    }
}


//This shouldn't get called yet.
void Server::killObjectConnection(const UUID& obj_id)
{
  if (mOSeg->getOSegType() == LOC_OSEG)
  {
    return;
  }
  //the rest assumes that we are dealing with a non-dummy implementation.


  //bftm change
  ObjectConnectionMap::iterator obj_map_it = mObjects.find(obj_id);
  if (obj_map_it != mObjects.end())
  {
    ObjectConnection* migrated_conn = mForwarder->removeObjectConnection(obj_id);
    mClosingConnections.insert(migrated_conn);
  }


  //  ObjectConnectionSet::iterator objConSetIt = mMigratingConnections.find(migrated_conn);
  ObjectConnectionMap::iterator objConMapIt = mMigratingConnections.find(obj_id);


  if (objConMapIt != mMigratingConnections.end())
    mMigratingConnections.erase(objConMapIt);

}



} // namespace CBR

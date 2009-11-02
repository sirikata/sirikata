
#ifndef _CBR_SERVER_HPP_
#define _CBR_SERVER_HPP_

#include "Utility.hpp"
#include "SpaceContext.hpp"

#include "ObjectHostConnectionManager.hpp"
#include "TimeProfiler.hpp"
#include "PollingService.hpp"

namespace CBR
{
class Forwarder;

class LocationService;
class Proximity;
class MigrationMonitor;

class CoordinateSegmentation;
class ObjectSegmentation;

class ObjectConnection;
class ObjectHostConnectionManager;

class ServerIDMap;

  /** Handles all the basic services provided for objects by a server,
   *  including routing and message delivery, proximity services, and
   *  object -> server mapping.  This is a singleton for each simulated
   *  server.  Other servers are referenced by their ServerID.
   */
class Server : public MessageRecipient, public PollingService
  {
  public:
      Server(SpaceContext* ctx, Forwarder* forwarder, LocationService* loc_service, CoordinateSegmentation* cseg, Proximity* prox, ObjectSegmentation* oseg, Address4* oh_listen_addr);
    ~Server();

      virtual void receiveMessage(Message* msg);
private:

    // PollingService Implementation
    virtual void poll();
    virtual void shutdown();

    // Methods for periodic servicing
    void serviceObjectHostNetwork();
    void checkObjectMigrations();

    // Finds the ObjectConnection associated with the given object, returns NULL if the object isn't available.
    ObjectConnection* getObjectConnection(const UUID& object_id) const;

    // Callback which handles messages from object hosts -- mostly just does sanity checking
    // before using the forwarder to do routing.
    void handleObjectHostMessage(const ObjectHostConnectionManager::ConnectionID& conn_id, CBR::Protocol::Object::ObjectMessage* msg);

    // Handle Session messages from an object
    void handleSessionMessage(const ObjectHostConnectionManager::ConnectionID& oh_conn_id, const CBR::Protocol::Object::ObjectMessage& msg);
    // Handle Connect message from object
    void handleConnect(const ObjectHostConnectionManager::ConnectionID& oh_conn_id, const CBR::Protocol::Object::ObjectMessage& container, const CBR::Protocol::Session::Connect& connect_msg);
    void handleConnect2(const ObjectHostConnectionManager::ConnectionID& oh_conn_id, const CBR::Protocol::Object::ObjectMessage& container, const CBR::Protocol::Session::Connect& connect_msg);
    // Handle Migrate message from object
    void handleMigrate(const ObjectHostConnectionManager::ConnectionID& oh_conn_id, const CBR::Protocol::Object::ObjectMessage& container, const CBR::Protocol::Session::Connect& migrate_msg);

    // Performs actual migration after all the necessary information is available.
    void handleMigration(const UUID& obj_id);

    //finally deletes any object connections to obj_id
    void killObjectConnection(const UUID& obj_id);

    void finishAddObject(const UUID& obj_id);

    bool checkAlreadyMigrating(const UUID& obj_id);
    void processAlreadyMigrating(const UUID& obj_id);

    SpaceContext* mContext;
    LocationService* mLocationService;
    CoordinateSegmentation* mCSeg;
    Proximity* mProximity;
    ObjectSegmentation* mOSeg;
    Forwarder* mForwarder;
    MigrationMonitor* mMigrationMonitor;


    ObjectHostConnectionManager* mObjectHostConnectionManager;

    typedef std::map<UUID, ObjectConnection*> ObjectConnectionMap;

    ObjectConnectionMap mObjects;
    ObjectConnectionMap mObjectsAwaitingMigration;

    typedef std::map<UUID, CBR::Protocol::Migration::MigrationMessage*> ObjectMigrationMap;
    ObjectMigrationMap mObjectMigrations;

    typedef std::set<ObjectConnection*> ObjectConnectionSet;
    ObjectConnectionSet mClosingConnections; // Connections that are closing but need to finish delivering some messages

    //std::map<UUID,ObjectConnection*>
    struct MigratingObjectConnectionsData
    {
      ObjectConnection* obj_conner;
      int milliseconds;
      ServerID migratingTo;
      TimedMotionVector3f loc;
      BoundingSphere3f bnds;
      uint64 uniqueConnId;
      bool serviceConnection;
    };

      typedef std::queue<Message*> MigrateMessageQueue;
      // Outstanding MigrateMessages, which get objects to other servers.
      MigrateMessageQueue mMigrateMessages;

    //    ObjectConnectionMap mMigratingConnections;//bftm add
    typedef std::map<UUID,MigratingObjectConnectionsData> MigConnectionsMap;
    MigConnectionsMap mMigratingConnections;//bftm add
    Timer mMigrationTimer;

    struct StoredConnection
    {
      ObjectHostConnectionManager::ConnectionID    conn_id;
      CBR::Protocol::Session::Connect             conn_msg;
    };

    typedef std::map<UUID, StoredConnection> StoredConnectionMap;
    StoredConnectionMap  mStoredConnectionData;
}; // class Server

} // namespace CBR

#endif //_CBR_SERVER_HPP_

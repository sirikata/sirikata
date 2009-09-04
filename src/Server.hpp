
#ifndef _CBR_SERVER_HPP_
#define _CBR_SERVER_HPP_

#include "Utility.hpp"
#include "SpaceContext.hpp"

#include "ObjectSegmentation.hpp"

namespace CBR
{
class Forwarder;

class LocationService;
class Proximity;

class CoordinateSegmentation;

// FIXME these are only passed to the forwarder...
class ServerMessageQueue;
class ObjectMessageQueue;

class LoadMonitor;

class ObjectConnection;
class ObjectConnectionManager;

class ServerIDMap;

  /** Handles all the basic services provided for objects by a server,
   *  including routing and message delivery, proximity services, and
   *  object -> server mapping.  This is a singleton for each simulated
   *  server.  Other servers are referenced by their ServerID.
   */
class Server : public MessageRecipient, public ObjectMessageRecipient
  {
  public:
      Server(SpaceContext* ctx, Forwarder* forwarder, LocationService* loc_service, CoordinateSegmentation* cseg, Proximity* prox, ObjectMessageQueue* omq, ServerMessageQueue* smq, LoadMonitor* lm, ObjectSegmentation* oseg, ServerIDMap* sidmap);
    ~Server();

      void service();

    ServerID lookup(const Vector3f&);
    ServerID lookup(const UUID&);

      // FIXME this should come form the network, currently just this way because ObjectHosts
      // aren't actually networked yet
      void handleOpenConnection(ObjectConnection* conn);

      virtual void receiveMessage(Message* msg);
      virtual void receiveMessage(const CBR::Protocol::Object::ObjectMessage& msg);
private:
      void handleObjectHostMessage(CBR::Protocol::Object::ObjectMessage* msg);

    void serviceProximity();
    void serviceNetwork();
    void checkObjectMigrations();

    // Handle Session messages from an object
    void handleSessionMessage(ObjectConnection* conn, const std::string& session_payload);
    // Handle Connect message from object
    void handleConnect(ObjectConnection* conn, const CBR::Protocol::Session::Connect& connect_msg);
    // Handle Migrate message from object
    void handleMigrate(ObjectConnection* conn, const CBR::Protocol::Session::Migrate& migrate_msg);

    void handleMigration(const UUID& obj_id);

      SpaceContext* mContext;
    LocationService* mLocationService;
    CoordinateSegmentation* mCSeg;
    Proximity* mProximity;
    ObjectSegmentation* mOSeg;
    Forwarder* mForwarder;

      ObjectConnectionManager* mObjectConnectionManager;

    typedef std::map<UUID, ObjectConnection*> ObjectConnectionMap;

    ObjectConnectionMap mObjects;
    ObjectConnectionMap mObjectsAwaitingMigration;

    typedef std::map<UUID, CBR::Protocol::Migration::MigrationMessage*> ObjectMigrationMap;
      ObjectMigrationMap mObjectMigrations;

    ObjectConnectionMap mConnectingObjects;

    typedef std::set<ObjectConnection*> ObjectConnectionSet;
    ObjectConnectionSet mClosingConnections; // Connections that are closing but need to finish delivering some messages
}; // class Server

} // namespace CBR

#endif //_CBR_SERVER_HPP_

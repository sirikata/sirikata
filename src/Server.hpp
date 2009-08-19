
#ifndef _CBR_SERVER_HPP_
#define _CBR_SERVER_HPP_

#include "Utility.hpp"
#include "LocationService.hpp"
#include "Network.hpp"
#include "ServerNetwork.hpp"
#include "ForwarderUtilityClasses.hpp"
#include "Forwarder.hpp"

#include "ObjectSegmentation.hpp"

namespace CBR
{

  class Proximity;
  class Object;
  class ServerIDMap;
  class CoordinateSegmentation;
  class Message;
  class ObjectToObjectMessage;
  class ServerMessageQueue;
  class ObjectMessageQueue;
  class Network;
  class Trace;
  class MigrateMessage;
  class LoadStatusMessage;
  class LoadMonitor;
  class Forwarder;

  /** Handles all the basic services provided for objects by a server,
   *  including routing and message delivery, proximity services, and
   *  object -> server mapping.  This is a singleton for each simulated
   *  server.  Other servers are referenced by their ServerID.
   */
class Server : public MessageRecipient, public ObjectMessageRecipient
  {
  public:
      Server(ServerID id, Forwarder* forwarder, LocationService* loc_service, CoordinateSegmentation* cseg, Proximity* prox, ObjectMessageQueue* omq, ServerMessageQueue* smq, LoadMonitor* lm, Trace* trace, ObjectSegmentation* oseg);
    ~Server();

    const ServerID& id() const;

    void tick(const Time& t);

    ServerID lookup(const Vector3f&);
    ServerID lookup(const UUID&);

      // FIXME this should come form the network, currently just this way because ObjectHosts
      // aren't actually networked yet
      void handleOpenConnection(ObjectConnection* conn);
      // FIXME this should come from the network instead of directly from ObjectHost
      // Note that this method should *only* be used for messages from the object host
      // ObjectMessages from other space servers should arrive via the normal ServerMessage
      // pipeline
      bool receiveObjectHostMessage(const std::string& msg);

      virtual void receiveMessage(Message* msg);
      virtual void receiveMessage(const CBR::Protocol::Object::ObjectMessage& msg);
  private:
    void proximityTick(const Time& t);
    void networkTick(const Time& t);
    void checkObjectMigrations();

    // Handle Session messages from an object
    void handleSessionMessage(ObjectConnection* conn, const std::string& session_payload);
    // Handle Connect message from object
    void handleConnect(ObjectConnection* conn, const CBR::Protocol::Session::Connect& connect_msg);
    // Handle Migrate message from object
    void handleMigrate(ObjectConnection* conn, const CBR::Protocol::Session::Migrate& migrate_msg);

    void handleMigration(const UUID& obj_id);

    ServerID mID;
    LocationService* mLocationService;
    CoordinateSegmentation* mCSeg;
    Proximity* mProximity;
    Time mCurrentTime;
    Trace* mTrace;
    ObjectSegmentation* mOSeg;
    Forwarder* mForwarder;

    typedef std::map<UUID, ObjectConnection*> ObjectConnectionMap;

    ObjectConnectionMap mObjects;
    ObjectConnectionMap mObjectsAwaitingMigration;

    typedef std::map<UUID, CBR::Protocol::Migration::MigrationMessage*> ObjectMigrationMap;
      ObjectMigrationMap mObjectMigrations;

    ObjectConnectionMap mConnectingObjects;
}; // class Server

} // namespace CBR

#endif //_CBR_SERVER_HPP_


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
class Server : public MessageRecipient
  {
  public:
      Server(ServerID id, Forwarder* forwarder, LocationService* loc_service, CoordinateSegmentation* cseg, Proximity* prox, ObjectMessageQueue* omq, ServerMessageQueue* smq, LoadMonitor* lm, Trace* trace, ObjectSegmentation* oseg);
    ~Server();

    const ServerID& id() const;

    void tick(const Time& t);

    ServerID lookup(const Vector3f&);
    ServerID lookup(const UUID&);

    // Handle Connect message from object
    void connect(ObjectConnection* conn, const std::string& connectPayload);
    // Handle Migrate message from object
    void migrate(ObjectConnection* conn, const std::string& migratePayload);

      virtual void receiveMessage(Message* msg);
  private:
    void proximityTick(const Time& t);
    void networkTick(const Time& t);
    void checkObjectMigrations();

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
}; // class Server

} // namespace CBR

#endif //_CBR_SERVER_HPP_

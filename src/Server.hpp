
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

      /** Sets up a connection for the given object. Can handle migrations by waiting
       *  to truly connect the object until the migration data has arrived.
       */
      void connect(Object* obj, bool wait_for_migrate = true);

      virtual void receiveMessage(Message* msg);
  private:
    void proximityTick(const Time& t);
    void networkTick(const Time& t);
    void checkObjectMigrations();

    MigrateMessage* wrapObjectStateForMigration(Object* obj);




    ServerID mID;
    LocationService* mLocationService;
    CoordinateSegmentation* mCSeg;
    Proximity* mProximity;
    Time mCurrentTime;
    Trace* mTrace;
    ObjectSegmentation* mOSeg;
    Forwarder* mForwarder;

    ObjectMap mObjects;
      ObjectMap mObjectsAwaitingMigration;

}; // class Server

} // namespace CBR

#endif //_CBR_SERVER_HPP_

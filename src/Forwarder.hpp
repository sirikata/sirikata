#ifndef _CBR_FORWARDER_HPP_
#define _CBR_FORWARDER_HPP_



#include "Utility.hpp"
#include "Time.hpp"
#include "Message.hpp"
#include "Network.hpp"
#include "ServerNetwork.hpp"

#include "ForwarderUtilityClasses.hpp"


namespace CBR
{
  class Object;
  class ObjectFactory;
  class ServerIDMap;
class LocationService;
class ObjectSegmentation;
  class CoordinateSegmentation;
  class ServerMessageQueue;
  class ObjectMessageQueue;
  class Network;
  class Trace;
  class LoadMonitor;

class Forwarder : public MessageDispatcher, public MessageRouter
  {
    private:
    //Unique to forwarder
      std::deque<SelfMessage> mSelfMessages; //will be used in route.
      std::deque<OutgoingMessage> mOutgoingMessages; //will be used in route.


    //Shared with server
      Trace* mTrace;  //will be used in several places.
      CoordinateSegmentation* mCSeg;
      ObjectSegmentation* mOSeg;
      LocationService* mLocationService;
      ObjectFactory* mObjectFactory;
      ObjectMessageQueue* mObjectMessageQueue;
      ServerMessageQueue* mServerMessageQueue;
      LoadMonitor* mLoadMonitor;
      ObjectMap* mObjects; //will be used in object() lookup call and possibly in migrate
      ServerID m_serv_ID;//Keeps copy of server id on forwarder.  necessary for sending messages to objects.

      Time* mCurrentTime;

      std::map<UUID,std::vector<Message*> > mObjectsInTransit;

      Time mLastSampleTime;
      Duration mSampleRate;

    //Private Functions
      void processChunk(const Sirikata::Network::Chunk&chunk, const ServerID& source_server, bool forwarded_self_msg);

      // Delivery interface.  This should be used to deliver received messages to the correct location -
      // the server or object it is addressed to.
      bool deliver(Message* msg);

      // Forward the given message to its proper server.  Use this when a message arrives and the object
      // no longer lives on this server.
      void forward(Message* msg, const UUID& dest);

      ServerID lookup(const Vector3f&); //returns the server id that is responsible for the 3d point Vector3f
      ServerID lookup(const UUID&); //


      // Get the object pointer for the given ID, or NULL if it isn't available on this server.
      Object* object(const UUID& dest) const;

      void tickOSeg(const Time&t);
      void getOSegMessages();

      const ServerID& serv_id() const; //used to access server id of the processes




    public:
      Forwarder(ServerID id);
      ~Forwarder(); //D-E-S-T-R-U-C-T-O-R
      void initialize(Trace* trace, CoordinateSegmentation* cseg, ObjectSegmentation* oseg, LocationService* locService, ObjectFactory* objectFactory, ObjectMessageQueue* omq, ServerMessageQueue* smq, LoadMonitor* lm, ObjectMap* objMap, Time* currTime);

      void tick(const Time&t);


      // Routing interface for servers.  This is used to route messages that originate from
      // a server provided service, and thus don't have a source object.  Messages may be destined
      // for either servers or objects.  The second form will simply automatically do the destination
      // server lookup.
      // if forwarding is true the message will be stuck onto a queue no matter what, otherwise it may be delivered directly
      void route(Message* msg, const ServerID& dest_server, bool is_forward = false);
      void route(Message* msg, const UUID& dest_obj, bool is_forward = false);


  };//end class Forwarder

} //end namespace CBR


#endif //_CBR_FORWARDER_HPP_

#ifndef _CBR_FORWARDER_HPP_
#define _CBR_FORWARDER_HPP_



#include "Utility.hpp"
#include "SpaceContext.hpp"
#include "Message.hpp"
#include "Network.hpp"

#include "Queue.hpp"
#include "FairQueue.hpp"

#include "TimeProfiler.hpp"

#include "PollingService.hpp"

#include "OSegLookupQueue.hpp"

#include "ServerMessageQueue.hpp"
#include "ServerMessageReceiver.hpp"

namespace CBR
{
  class Object;
  class ServerIDMap;
  class ObjectSegmentation;
  class CoordinateSegmentation;
  class Network;
  class Trace;
  class ObjectConnection;
  class OSegLookupQueue;
class ForwarderServiceQueue;

class Forwarder : public MessageDispatcher, public MessageRouter, public MessageRecipient, public PollingService,
                  public ServerMessageQueue::Listener, public ServerMessageReceiver::Listener
{
private:
    SpaceContext* mContext;
    ForwarderServiceQueue *mOutgoingMessages;
    ServerMessageQueue* mServerMessageQueue;
    ServerMessageReceiver* mServerMessageReceiver;

    OSegLookupQueue* mOSegLookups; //this maps the object ids to a list of messages that are being looked up in oseg.


    // Object connections, identified by a separate unique ID to handle fast migrations
    uint64 mUniqueConnIDs; // Connection ID generator
    struct UniqueObjConn
    {
      uint64 id;
      ObjectConnection* conn;
    };
    typedef std::map<UUID, UniqueObjConn> ObjectConnectionMap;
    ObjectConnectionMap mObjectConnections;

    typedef std::vector<ServerID> ListServersUpdate;
    typedef std::map<UUID,ListServersUpdate> ObjectServerUpdateMap;
    ObjectServerUpdateMap mServersToUpdate; // Map of object id -> servers which should be notified of new result


    // -- Boiler plate stuff - initialization, destruction, methods to satisfy interfaces
  public:
      Forwarder(SpaceContext* ctx);
      ~Forwarder();
    void initialize(ObjectSegmentation* oseg, ServerMessageQueue* smq, ServerMessageReceiver* smr, uint32 oseg_lookup_queue_size);

  protected:

    virtual void dispatchMessage(Message* msg) const;
    virtual void dispatchMessage(const CBR::Protocol::Object::ObjectMessage& msg) const;

  private:
    virtual void poll();

    // -- Public routing interface
  public:
    WARN_UNUSED
    bool route(MessageRouter::SERVICES svc, Message* msg);

    WARN_UNUSED
    bool route(CBR::Protocol::Object::ObjectMessage* msg);

    // -- Real routing interface + implementation


    // --- Inputs
  public:
    // Received from OH networking, needs forwarding decision.  Forwards or
    // drops -- ownership is given to Forwarder either way
    void routeObjectHostMessage(CBR::Protocol::Object::ObjectMessage* obj_msg);
  private:
    // Received from other space server, needs forwarding decision
    void receiveMessage(Message* msg);

  private:
    // --- Worker Methods - do the real forwarding decision making and work

    /** Try to forward a message to get it closer to the destination object.
     *  This checks if we have a direct connection to the object, then does an
     *  OSeg lookup if necessary.
     */
    WARN_UNUSED
    bool forward(CBR::Protocol::Object::ObjectMessage* msg, ServerID forwardFrom = NullServerID);

    // This version is provided if you already know which server the message should be sent to
    WARN_UNUSED
    bool routeObjectMessageToServer(CBR::Protocol::Object::ObjectMessage* msg, ServerID dest_serv, OSegLookupQueue::ResolvedFrom resolved_from, ServerID forwardFrom = NullServerID);

    // Services forwarder queues
    // FIXME this should be changed to be fully event driven
    void serviceSendQueues();
    // Try to pull messages from the Forwarder queue and push them into the
    // ServerMessageQueue
    void trySendToServer(ServerID sid);

    // ServerMessageQueue::Listener Interface
    virtual void serverMessageSent(Message* msg);
    // ServerMessageReceiver::Listener Interface
    virtual void serverMessageReceived(Message* msg);

    // -- Object Connection Management used by Server
  public:
    void addObjectConnection(const UUID& dest_obj, ObjectConnection* conn);
    void enableObjectConnection(const UUID& dest_obj);
    ObjectConnection* removeObjectConnection(const UUID& dest_obj);
  //private: FIXME these should not be public
  public:
    ObjectConnection* getObjectConnection(const UUID& dest_obj);
    ObjectConnection* getObjectConnection(const UUID& dest_obj, uint64& uniqueconnid );
}; // class Forwarder

} //end namespace CBR


#endif //_CBR_FORWARDER_HPP_

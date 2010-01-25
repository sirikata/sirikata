#ifndef _CBR_FORWARDER_HPP_
#define _CBR_FORWARDER_HPP_



#include "Utility.hpp"
#include "SpaceContext.hpp"
#include "Message.hpp"
#include "Network.hpp"

#include "Queue.hpp"
#include "FairQueue.hpp"

#include "TimeProfiler.hpp"

#include "OSegLookupQueue.hpp"

#include "ServerMessageQueue.hpp"
#include "ServerMessageReceiver.hpp"

#include "SSTImpl.hpp"

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
class ServerWeightCalculator;

class Forwarder : public ServerMessageDispatcher, public ObjectMessageDispatcher,
		    public ServerMessageRouter, public ObjectMessageRouter,
                    public MessageRecipient,
                    public ServerMessageQueue::Sender,
                    public ServerMessageReceiver::Listener
{
private:
    SpaceContext* mContext;
    ForwarderServiceQueue *mOutgoingMessages;
    ServerWeightCalculator* mServerWeightCalculator;
    ServerMessageQueue* mServerMessageQueue;
    ServerMessageReceiver* mServerMessageReceiver;

    OSegLookupQueue* mOSegLookups; //this maps the object ids to a list of messages that are being looked up in oseg.
    boost::shared_ptr<BaseDatagramLayer<UUID> >  mSSTDatagramLayer;


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
    void initialize(ObjectSegmentation* oseg, ServerWeightCalculator* swc, ServerMessageQueue* smq, ServerMessageReceiver* smr, uint32 oseg_lookup_queue_size);

  protected:

    virtual void dispatchMessage(Message* msg) const;
    virtual void dispatchMessage(const CBR::Protocol::Object::ObjectMessage& msg) const;

  private:
    // -- Public routing interface
  public:
    WARN_UNUSED
    bool route(ServerMessageRouter::SERVICES svc, Message* msg);

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

    // FIXME this is not the ideal place for this -- checks if dest queue/weight
    // has been added to ServerMessageQueue, adds it if its missing
    void checkDestWeight(ServerID sid);
    std::set<ServerID> mSetDests;


    // ServerMessageQueue::Sender Interface
    virtual Message* serverMessageFront(ServerID dest);
    virtual Message* serverMessagePull(ServerID dest);
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

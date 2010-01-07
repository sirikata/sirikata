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

namespace CBR
{
  class Object;
  class ServerIDMap;
  class ObjectSegmentation;
  class CoordinateSegmentation;
  class ServerMessageQueue;
  class ServerMessageReceiver;
  class Network;
  class Trace;
  class ObjectConnection;
  class OSegLookupQueue;

class ForwarderQueue{
public:
    typedef FairQueue<Message, MessageRouter::SERVICES, AbstractQueue<Message*> > OutgoingFairQueue;
    std::vector<OutgoingFairQueue*> mQueues;
    uint32 mQueueSize;
    ServerMessageQueue *mServerMessageQueue;
    ForwarderQueue(ServerMessageQueue*smq, uint32 size){
        mServerMessageQueue=smq;
        mQueueSize=size;
    }
    size_t numServerQueues()const {
        return mQueues.size();
    }
    OutgoingFairQueue& getFairQueue(ServerID sid) {
        while (mQueues.size()<=sid) {
            mQueues.push_back(NULL);
        }
        if(!mQueues[sid]) {
            mQueues[sid]=new OutgoingFairQueue();
            for(unsigned int i=0;i<MessageRouter::NUM_SERVICES;++i) {
                mQueues[sid]->addQueue(new Queue<Message*>(mQueueSize),(MessageRouter::SERVICES)i,1.0);
            }
        }
        return *mQueues[sid];
    }
    ~ForwarderQueue (){
        for(unsigned int i=0;i<mQueues.size();++i) {
            if(mQueues[i]) {
                delete mQueues[i];
            }
        }
    }
};

class ForwarderSampler;
class Forwarder : public MessageDispatcher, public MessageRouter, public MessageRecipient, public PollingService
{
private:
    SpaceContext* mContext;
    ForwarderQueue *mOutgoingMessages;
    ServerMessageQueue* mServerMessageQueue;
    ServerMessageReceiver* mServerMessageReceiver;

    OSegLookupQueue* mOSegLookups; //this maps the object ids to a list of messages that are being looked up in oseg.

    ForwarderSampler* mSampler;


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

    TimeProfiler::Stage* mForwarderQueueStage;
    TimeProfiler::Stage* mReceiveStage;

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
    // FIXME this one should be relatively easy to make event driven based on
    // network input
    void serviceReceiveQueues();

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

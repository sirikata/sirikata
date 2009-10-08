#ifndef _CBR_FORWARDER_HPP_
#define _CBR_FORWARDER_HPP_



#include "Utility.hpp"
#include "SpaceContext.hpp"
#include "Message.hpp"
#include "Network.hpp"
#include "ServerNetwork.hpp"

#include "ForwarderUtilityClasses.hpp"
#include "Queue.hpp"
#include "FairQueue.hpp"

#include "TimeProfiler.hpp"

namespace CBR
{
  class Object;
  class ServerIDMap;
  class ObjectSegmentation;
  class CoordinateSegmentation;
  class ServerMessageQueue;
  class Network;
  class Trace;
  class ObjectConnection;


class ForwarderQueue{
public:
    class CanSendPredicate{
        ServerMessageQueue* mServerMessageQueue;
    public:
        CanSendPredicate(ServerMessageQueue* s){
            mServerMessageQueue=s;
        }
        bool operator() (MessageRouter::SERVICES svc, const Message* msg);
    };
    typedef FairQueue<Message, MessageRouter::SERVICES, AbstractQueue<Message*>, AlwaysUsePredicate, true> OutgoingFairQueue;
    std::vector<OutgoingFairQueue*> mQueues;
    uint32 mQueueSize;
    ServerMessageQueue *mServerMessageQueue;
    ForwarderQueue(ServerMessageQueue*smq, uint32 size){
        mServerMessageQueue=smq;
    }
    size_t numServerQueues()const {
        return mQueues.size();
    }
    OutgoingFairQueue& getFairQueue(ServerID sid) {
        while (mQueues.size()<=sid) {
            mQueues.push_back(NULL);
        }
        if(!mQueues[sid]) {
            mQueues[sid]=new OutgoingFairQueue(AlwaysUsePredicate());
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
class Forwarder : public MessageDispatcher, public MessageRouter, public MessageRecipient
{
    private:
    //Unique to forwarder
      std::deque<SelfMessage> mSelfMessages; //will be used in route.
      ForwarderQueue *mOutgoingMessages;

      SpaceContext* mContext;

    //Shared with server
      CoordinateSegmentation* mCSeg;
      ObjectSegmentation* mOSeg;
      ServerMessageQueue* mServerMessageQueue;

      uint64 mUniqueConnIDs;


    struct MessageAndForward
    {
      bool mIsForward;
      CBR::Protocol::Object::ObjectMessage* mMessage;
    };
    typedef std::vector<MessageAndForward> ObjectMessageList;
    //    typedef std::vector<CBR::Protocol::Object::ObjectMessage*> ObjectMessageList;
    std::map<UUID,ObjectMessageList> mObjectsInTransit;  //this is a map of messages's for objects that are being looked up in oseg or are in transit.



    OSegLookupQueue  queueMap; //this maps the object ids to a list of messages that are being looked up in oseg.



    Time mLastSampleTime;
    Duration mSampleRate;


    struct UniqueObjConn
    {
      uint64 id;
      ObjectConnection* conn;
    };


  //    typedef std::map<UUID, ObjectConnection*> ObjectConnectionMap;
  //    ObjectConnectionMap mObjectConnections;

    typedef std::map<UUID, UniqueObjConn> ObjectConnectionMap;
    ObjectConnectionMap mObjectConnections;


    TimeProfiler mProfiler;

    //Private Functions
    void processChunk(Message* msg, bool forwarded_self_msg);


    typedef std::vector<ServerID> ListServersUpdate;
    typedef std::map<UUID,ListServersUpdate> ObjectServerUpdateMap;
    ObjectServerUpdateMap mServersToUpdate;


protected:
    virtual void dispatchMessage(Message* msg) const;
    virtual void dispatchMessage(const CBR::Protocol::Object::ObjectMessage& msg) const;

    public:
      Forwarder(SpaceContext* ctx);
      ~Forwarder(); //D-E-S-T-R-U-C-T-O-R
      void initialize(CoordinateSegmentation* cseg, ObjectSegmentation* oseg, ServerMessageQueue* smq);

      void service();

      void tickOSeg(const Time&t);


      // Routing interface for servers.  This is used to route messages that originate from
      // a server provided service, and thus don't have a source object.  Messages may be destined
      // for either servers or objects.  The second form will simply automatically do the destination
      // server lookup.
      // if forwarding is true the message will be stuck onto a queue no matter what, otherwise it may be delivered directly
    //__attribute__ ((warn_unused_result))
    bool route(MessageRouter::SERVICES svc, Message* msg, bool is_forward = false);

    //note: whenever we're forwarding a message from another object, we'll want to include the forwardFrom ServerID so that we can send an oseg update message to the server with
    //the stale cache value.
    __attribute__ ((warn_unused_result))
     bool route(CBR::Protocol::Object::ObjectMessage* msg, bool is_forward = false, ServerID forwardFrom = NullServerID);
  //  void route(CBR::Protocol::Object::ObjectMessage* msg, bool is_forward, ServerID forwardFrom );

  //  private:
      // This version is provided if you already know which server the message should be sent to
    __attribute__ ((warn_unused_result))
    bool routeObjectMessageToServer(CBR::Protocol::Object::ObjectMessage* msg, ServerID dest_serv, bool is_forward=false, MessageRouter::SERVICES from_another_object=MessageRouter::OBJECT_MESSAGESS);
  public:
      bool routeObjectHostMessage(CBR::Protocol::Object::ObjectMessage* obj_msg);

      void receiveMessage(Message* msg);

      void addObjectConnection(const UUID& dest_obj, ObjectConnection* conn);
      ObjectConnection* removeObjectConnection(const UUID& dest_obj);
      ObjectConnection* getObjectConnection(const UUID& dest_obj);
      ObjectConnection* getObjectConnection(const UUID& dest_obj, uint64& uniqueconnid );
  };//end class Forwarder

} //end namespace CBR


#endif //_CBR_FORWARDER_HPP_

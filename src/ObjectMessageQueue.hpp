#ifndef _CBR_OBJECT_MESSAGE_QUEUE_HPP
#define _CBR_OBJECT_MESSAGE_QUEUE_HPP

#include "Utility.hpp"
#include "Statistics.hpp"
#include "ObjectSegmentation.hpp"

namespace CBR{
class ServerMessageQueue;


struct ObjMessQBeginSend
{
  UUID src_uuid;
  UUID dest_uuid;

  UniqueMessageID msg_id;
  Network::Chunk msg_serialized;
};


  
class ObjectMessageQueue {
public:
    ObjectMessageQueue(ServerMessageQueue*sm, ObjectSegmentation* oseg, Trace* trace)
      : mServerMessageQueue(sm),
        mOSeg(oseg),
        mTrace(trace)
    {}

    virtual ~ObjectMessageQueue(){}
  //    virtual bool send(CBR::Protocol::Object::ObjectMessage* msg) = 0;

  virtual bool beginSend(CBR::Protocol::Object::ObjectMessage* msg, ObjMessQBeginSend& ) = 0;
  virtual bool endSend(const ObjMessQBeginSend&, ServerID dest_server_id) = 0;
  
    virtual void service(const Time& t)=0;

    virtual void registerClient(const UUID& oid, float weight=1) = 0;
    virtual void unregisterClient(const UUID& oid) = 0;
protected:
  //    ServerID lookup(const UUID& obj_id) {
  //        return mOSeg->lookup(obj_id);
  //    }

  //bftm added
  void serviceOSeg(const Time&t, std::map<UUID,ServerID>& updated){
    mOSeg->tick(t, updated);
  }
    
  //end bftm added

  
    ServerMessageQueue *mServerMessageQueue;
    ObjectSegmentation* mOSeg;
    Trace* mTrace;
};
}
#endif

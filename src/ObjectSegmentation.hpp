#ifndef _CBR_OBJECT_SEGMENTATION_HPP_
#define _CBR_OBJECT_SEGMENTATION_HPP_

#include "Utility.hpp"
#include "Statistics.hpp"
#include "Message.hpp"
#include "Object.hpp"
#include "Statistics.hpp"
#include "Time.hpp"

//object segmenter h file

namespace CBR
{


   const ServerID OBJECT_IN_TRANSIT = -999;

  
  class ObjectSegmentation
  {

  private:

  protected:
    ServerID mID;


    Trace* mTrace;

    
  public:
    virtual ServerID lookup(const UUID& obj_id) const = 0;
    virtual void osegChangeMessage(OSegChangeMessage*) = 0;
    //    virtual void tick(const Time& t, std::deque<OutgoingMessage> &messageQueueToPushTo) = 0;
    virtual void tick(const Time& t, std::map<UUID,ServerID> updated) = 0;
    virtual ~ObjectSegmentation() {}
    //    virtual void migrateMessage(MigrateMessage*) = 0;
    virtual void migrateObject(const UUID& obj_id, const ServerID new_server_id) = 0;
    virtual void addObject(const UUID& obj_id, const ServerID ourID) = 0;
    virtual void generateAcknowledgeMessage(Object* obj, ServerID sID_to, Message* returner)=0;

    virtual ServerID getHostServerID() = 0;

    
  };
}
#endif

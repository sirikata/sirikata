#ifndef _CBR_OBJECT_SEGMENTATION_HPP_
#define _CBR_OBJECT_SEGMENTATION_HPP_

#include "Utility.hpp"
#include "SpaceContext.hpp"
#include "Message.hpp"

//object segmenter h file

namespace CBR
{


  const ServerID OBJECT_IN_TRANSIT = -999;


  class ObjectSegmentation : public MessageRecipient
  {

  private:

  protected:
    SpaceContext* mContext;



  public:
      ObjectSegmentation(SpaceContext* ctx)
       : mContext(ctx)
      {
      }

      virtual ~ObjectSegmentation() {}

    virtual void lookup(const UUID& obj_id) = 0;// const = 0;
    virtual void service(std::map<UUID,ServerID>& updated) = 0;

    virtual void migrateObject(const UUID& obj_id, const ServerID new_server_id) = 0;

    virtual void addObject(const UUID& obj_id, const ServerID ourID) = 0;


    //    virtual  Message* generateAcknowledgeMessage(const UUID& obj_id,ServerID sID_to) = 0;
    //    virtual void addObject(const UUID& obj_id, const ServerID ourID, const ServerID) = 0;
    //    virtual void addObject(const UUID& obj_id, Object* obj, const ServerID idServerAckTo) = 0;
    //    virtual void processLookupMessage(OSegLookupMessage* msg) = 0; //no longer need to do this on account of not having oseg lookup messages.
    //    virtual Message* generateAcknowledgeMessage(Object* obj,ServerID sID_to) = 0;


  };
}
#endif

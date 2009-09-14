#ifndef _CBR_OBJECT_SEGMENTATION_HPP_
#define _CBR_OBJECT_SEGMENTATION_HPP_

#include "Utility.hpp"
#include "SpaceContext.hpp"
#include "Message.hpp"
#include <iostream>
#include <iomanip>
//object segmenter h file

#define LOC_OSEG   0
#define CRAQ_OSEG  1


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

    virtual ServerID lookup(const UUID& obj_id) = 0;
    virtual void service(std::map<UUID,ServerID>& updated) = 0;
    virtual void migrateObject(const UUID& obj_id, const ServerID new_server_id) = 0;
    virtual void addObject(const UUID& obj_id, const ServerID ourID, bool) = 0;

    virtual bool clearToMigrate(const UUID& obj_id) = 0; //

    virtual int getOSegType() = 0;

    
  };
}
#endif

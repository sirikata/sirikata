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
    virtual void addObject(const UUID& obj_id, const ServerID ourID, bool) = 0;

  };
}
#endif

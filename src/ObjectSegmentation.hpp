#ifndef _CBR_OBJECT_SEGMENTATION_HPP_
#define _CBR_OBJECT_SEGMENTATION_HPP_

#include "Utility.hpp"
#include "SpaceContext.hpp"
#include "Message.hpp"
#include "PollingService.hpp"
#include <iostream>
#include <iomanip>
//object segmenter h file

#define LOC_OSEG   0
#define CRAQ_OSEG  1


namespace CBR
{


  const ServerID OBJECT_IN_TRANSIT = -999;

/* Listener interface for OSeg events.
 *
 * Note that these are likely to be called from another thread, so
 * the implementing class must ensure they are thread safe.
 */
class OSegListener {
public:
    virtual void osegLookupCompleted(const UUID& id, const ServerID& dest) = 0;
}; // class OSegListener


class ObjectSegmentation : public MessageRecipient, public PollingService
  {

  private:

  protected:
    virtual void poll() = 0;

    SpaceContext* mContext;
      TimeProfiler::Stage* mServiceStage;
      OSegListener* mListener;
  public:

    ObjectSegmentation(SpaceContext* ctx)
     : PollingService(ctx->mainStrand),
       mContext(ctx),
       mListener(NULL)
    {
        mServiceStage = mContext->profiler->addStage("OSeg");
    }



    virtual ~ObjectSegmentation() {}

      void setListener(OSegListener* listener) {
          mListener = listener;
      }

    virtual ServerID lookup(const UUID& obj_id) = 0;
    virtual void migrateObject(const UUID& obj_id, const ServerID new_server_id) = 0;
    virtual void addObject(const UUID& obj_id, const ServerID ourID, bool) = 0;
    virtual void newObjectAdd(const UUID& obj_id) = 0;
    virtual bool clearToMigrate(const UUID& obj_id) = 0;
    virtual int getOSegType() = 0;


  };
}
#endif

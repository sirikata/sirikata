#ifndef _CBR_OBJECT_SEGMENTATION_HPP_
#define _CBR_OBJECT_SEGMENTATION_HPP_

#include "Utility.hpp"
#include "SpaceContext.hpp"
#include "Message.hpp"
#include "PollingService.hpp"
#include <iostream>
#include <iomanip>
#include "craq_oseg/asyncUtil.hpp"
//object segmenter h file

namespace CBR
{

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
    IOStrand* oStrand;


  public:


    ObjectSegmentation(SpaceContext* ctx,IOStrand* o_strand)
     : PollingService(o_strand),
       mContext(ctx),
       mListener(NULL),
       oStrand(o_strand)
    {
      fflush(stdout);
        mServiceStage = mContext->profiler->addStage("OSeg");
    }

    virtual ~ObjectSegmentation() {}

      void setListener(OSegListener* listener) {
          mListener = listener;
      }

    virtual ServerID lookup(const UUID& obj_id) = 0;
    virtual ServerID cacheLookup(const UUID& obj_id) = 0;
    virtual void migrateObject(const UUID& obj_id, const ServerID new_server_id) = 0;
    virtual void addObject(const UUID& obj_id, const ServerID ourID, bool) = 0;
    virtual void newObjectAdd(const UUID& obj_id) = 0;
    virtual bool clearToMigrate(const UUID& obj_id) = 0;
    virtual void craqGetResult(CraqOperationResult* cor) = 0; //also responsible for destroying
    virtual void craqSetResult(CraqOperationResult* cor) = 0; //also responsible for destroying
    virtual std::vector<PollingService*> getNestedPollers() = 0;


  };
}
#endif

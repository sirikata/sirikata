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
class OSegLookupListener {
public:
    virtual void osegLookupCompleted(const UUID& id, const CraqEntry& dest) = 0;
}; // class OSegLookupListener


/** Listener interface for OSeg write events. */
class OSegWriteListener {
public:
    virtual void osegWriteFinished(const UUID& id) = 0;
    virtual void osegMigrationAcknowledged(const UUID& id) = 0;
}; // class OSegMembershipListener



class ObjectSegmentation : public MessageRecipient, public Service
  {
  protected:
    SpaceContext* mContext;
    OSegLookupListener* mLookupListener;
      OSegWriteListener* mWriteListener;
    IOStrand* oStrand;


  public:


    ObjectSegmentation(SpaceContext* ctx,IOStrand* o_strand)
     : mContext(ctx),
       mLookupListener(NULL),
       mWriteListener(NULL),
       oStrand(o_strand)
    {
      fflush(stdout);
    }

    virtual ~ObjectSegmentation() {}

      virtual void start() {
      }

      void setLookupListener(OSegLookupListener* listener) {
          mLookupListener = listener;
      }

      void setWriteListener(OSegWriteListener* listener) {
          mWriteListener = listener;
      }

    virtual CraqEntry lookup(const UUID& obj_id) = 0;
    virtual CraqEntry cacheLookup(const UUID& obj_id) = 0;
    virtual void migrateObject(const UUID& obj_id, const CraqEntry& new_server_id) = 0;
      virtual void addObject(const UUID& obj_id, float radius, ServerID idServerAckTo, bool) = 0;
    virtual void newObjectAdd(const UUID& obj_id, float radius) = 0;
    virtual bool clearToMigrate(const UUID& obj_id) = 0;
    virtual void craqGetResult(CraqOperationResult* cor) = 0; //also responsible for destroying
    virtual void craqSetResult(CraqOperationResult* cor) = 0; //also responsible for destroying
    virtual std::vector<PollingService*> getNestedPollers() = 0;


  };
}
#endif

/*  Sirikata
 *  ObjectSegmentation.hpp
 *
 *  Copyright (c) 2010, Daniel Reiter Horn
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Sirikata nor the names of its contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _SIRIKATA_OBJECT_SEGMENTATION_HPP_
#define _SIRIKATA_OBJECT_SEGMENTATION_HPP_

#include <sirikata/cbrcore/Utility.hpp>
#include <sirikata/cbrcore/SpaceContext.hpp>
#include <sirikata/cbrcore/Message.hpp>
#include <sirikata/cbrcore/PollingService.hpp>
#include <iostream>
#include <iomanip>
#include "craq_oseg/asyncUtil.hpp"
#include <queue>
//object segmenter h file

namespace Sirikata
{

/* Listener interface for OSeg events.
 *
 * Note that these are likely to be called from another thread, so
 * the implementing class must ensure they are thread safe.
 */
class OSegLookupListener {
public:
    virtual ~OSegLookupListener() {}

    virtual void osegLookupCompleted(const UUID& id, const CraqEntry& dest) = 0;
}; // class OSegLookupListener


/** Listener interface for OSeg write events. */
class OSegWriteListener {
public:
    virtual ~OSegWriteListener() {}

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


  };
}
#endif

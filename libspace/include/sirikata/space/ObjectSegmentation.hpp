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

#include <sirikata/space/SpaceContext.hpp>
#include <sirikata/space/ServerMessage.hpp>
#include <sirikata/space/CoordinateSegmentation.hpp>
#include <sirikata/core/service/Service.hpp>
#include <sirikata/core/util/Factory.hpp>

#include <sirikata/core/queue/ThreadSafeQueueWithNotification.hpp>
#include "Protocol_OSeg.pbj.hpp"

namespace Sirikata
{

class OSegCache;

class OSegEntry {
protected:
    uint32 mServer;
    float mRadius;
public:
    explicit OSegEntry()
     : mServer(NullServerID), mRadius(0)
    {}
    OSegEntry(uint32 server, float radius) {
        mServer=server;
        mRadius=radius;
    }
    static OSegEntry null() {
        return OSegEntry(NullServerID,0);
    }
    bool isNull() const {
        return mServer==NullServerID&&mRadius==0;
    }
    bool notNull() const {
        return !isNull();
    }
    uint32 server() const{
        return mServer;
    }
    float radius () const{
        return mRadius;
    }
    void setServer(uint32 server)
    {
        mServer = server;
    }
    void setRadius(float radius)
    {
        mRadius = radius;
    }
};

/* Listener interface for OSeg events.
 *
 * Note that these are likely to be called from another thread, so
 * the implementing class must ensure they are thread safe.
 */
class OSegLookupListener {
public:
    virtual ~OSegLookupListener() {}

    virtual void osegLookupCompleted(const UUID& id, const OSegEntry& dest) = 0;
}; // class OSegLookupListener


/** Listener interface for OSeg write events. */
class OSegWriteListener {
public:
    virtual ~OSegWriteListener() {}

    virtual void osegWriteFinished(const UUID& id) = 0;
    virtual void osegMigrationAcknowledged(const UUID& id) = 0;
}; // class OSegMembershipListener




class SIRIKATA_SPACE_EXPORT ObjectSegmentation : public Service, public MessageRecipient
{
protected:
    SpaceContext* mContext;
    bool mStopping;
    OSegLookupListener* mLookupListener;
    OSegWriteListener* mWriteListener;
    Network::IOStrand* oStrand;

    Router<Message*>* mOSegServerMessageService;
    // This queue handles outgoing migration ack messages, getting them safely
    // into the outgoing queue. Implementations only need to call queueMigAck()
    // from any other thread to get messages sent.
    Sirikata::ThreadSafeQueueWithNotification<Message*> mMigAckMessages;
    Message* mFrontMigAck;
    void queueMigAck(const Sirikata::Protocol::OSeg::MigrateMessageAcknowledge& msg);
    void trySendMigAcks();
    void handleNewMigAckMessages();

    // MessageRecipient Interface
    virtual void receiveMessage(Message* msg);

    // These handlers for server-to-server messages need to be provided by
    // implementations. Note that these are *not* thread safe! You may need to
    // shift processing into your own thread/strand.
    virtual void handleMigrateMessageAck(const Sirikata::Protocol::OSeg::MigrateMessageAcknowledge& msg) = 0;
    virtual void handleUpdateOSegMessage(const Sirikata::Protocol::OSeg::UpdateOSegMessage& update_oseg_msg) = 0;

public:
    ObjectSegmentation(SpaceContext* ctx, Network::IOStrand* o_strand);
    virtual ~ObjectSegmentation();

      virtual void start() {
      }

      virtual void stop() {
          mStopping = true;
      }

      void setLookupListener(OSegLookupListener* listener) {
          mLookupListener = listener;
      }

      void setWriteListener(OSegWriteListener* listener) {
          mWriteListener = listener;
      }

    virtual OSegEntry lookup(const UUID& obj_id) = 0;
    virtual OSegEntry cacheLookup(const UUID& obj_id) = 0;
    virtual void migrateObject(const UUID& obj_id, const OSegEntry& new_server_id) = 0;
    virtual void addNewObject(const UUID& obj_id, float radius) = 0;
    virtual void addMigratedObject(const UUID& obj_id, float radius, ServerID idServerAckTo, bool) = 0;
    virtual void removeObject(const UUID& obj_id) = 0;
    virtual bool clearToMigrate(const UUID& obj_id) = 0;

    virtual int getPushback()
    {
        return 0;
    }

  };

class SIRIKATA_SPACE_EXPORT OSegFactory
    : public AutoSingleton<OSegFactory>,
      public Factory5<ObjectSegmentation*, SpaceContext*, Network::IOStrand*, CoordinateSegmentation*, OSegCache*, const String &>
{
  public:
    static OSegFactory& getSingleton();
    static void destroy();
}; // class OSegFactory

} // namespace Sirikata

#include <sirikata/space/OSegCache.hpp>

#endif

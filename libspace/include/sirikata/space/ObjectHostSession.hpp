// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_LIBSPACE_OBJECT_HOST_SESSION_HPP_
#define _SIRIKATA_LIBSPACE_OBJECT_HOST_SESSION_HPP_

#include <sirikata/space/Platform.hpp>
#include <sirikata/core/util/ListenerProvider.hpp>
#include <sirikata/core/ohdp/SSTDecls.hpp>
#include <sirikata/space/SpaceContext.hpp>

namespace Sirikata {

// These classes are thin wrappers around ObjectHostConnectionManager and it's
// affiliated classes for tracking sessions

class SIRIKATA_SPACE_EXPORT ObjectHostSession {
  public:
    ObjectHostSession(const OHDP::NodeID& id, OHDPSST::StreamPtr strm)
        : mID(id),
        mSSTStream(strm),
        mSeqNo(new SeqNo())
        {
        }

    ~ObjectHostSession();

    const OHDP::NodeID& id() const { return mID; }
    const OHDPSST::StreamPtr& stream() const { return mSSTStream; }
    const SeqNoPtr& seqNoPtr() const { return mSeqNo; }

  private:
    OHDP::NodeID mID;
    OHDPSST::StreamPtr mSSTStream;
    SeqNoPtr mSeqNo;
};
typedef std::tr1::shared_ptr<ObjectHostSession> ObjectHostSessionPtr;

class SIRIKATA_SPACE_EXPORT ObjectHostSessionListener {
  public:
    virtual ~ObjectHostSessionListener() {}

    virtual void onObjectHostSession(const OHDP::NodeID& id, ObjectHostSessionPtr) {}
    virtual void onObjectHostSessionEnded(const OHDP::NodeID& id) {}
};

/** Small class that acts as a Provider for ObjectHostSessionListeners, but just
 * forwards events from the real provider (allowing us to provide the
 * ObjectHostSessionManager before the real provider is created).
 */
class ObjectHostSessionManager : public Provider<ObjectHostSessionListener*> {
  public:
    ObjectHostSessionManager(SpaceContext* ctx) {
        ctx->mObjectHostSessionManager = this;
    }

    // Owner interface
    void fireObjectHostSession(const OHDP::NodeID& id, OHDPSST::StreamPtr oh_stream) {
        ObjectHostSessionPtr sess(new ObjectHostSession(id, oh_stream));
        mOHSessions[id] = sess;
        notify(&ObjectHostSessionListener::onObjectHostSession, id, sess);

    }
    void fireObjectHostSessionEnded(const OHDP::NodeID& id) {
        notify(&ObjectHostSessionListener::onObjectHostSessionEnded, id);
        ObjectHostSessionMap::const_iterator it = mOHSessions.find(id);
        if (it != mOHSessions.end())
            mOHSessions.erase(it);
    }


    // User interface
    ObjectHostSessionPtr getSession(const OHDP::NodeID& id) {
        ObjectHostSessionMap::const_iterator it = mOHSessions.find(id);
        if (it == mOHSessions.end()) return ObjectHostSessionPtr();
        return it->second;
    }

  private:
    typedef std::tr1::unordered_map<OHDP::NodeID, ObjectHostSessionPtr, OHDP::NodeID::Hasher> ObjectHostSessionMap;
    ObjectHostSessionMap mOHSessions;
};

} // namespace Sirikata

#endif //_SIRIKATA_LIBSPACE_OBJECT_HOST_SESSION_HPP_

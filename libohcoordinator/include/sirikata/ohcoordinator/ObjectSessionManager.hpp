/*  Sirikata
 *  ObjectSessionManager.hpp
 *
 *  Copyright (c) 2010, Ewen Cheslack-Postava
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

#ifndef _SIRIKATA_SPACE_OBJECT_SESSION_MANAGER_HPP_
#define _SIRIKATA_SPACE_OBJECT_SESSION_MANAGER_HPP_

#include <sirikata/ohcoordinator/Platform.hpp>
#include <sirikata/core/odp/SST.hpp>
#include <sirikata/core/util/ListenerProvider.hpp>
#include <sirikata/ohcoordinator/SpaceContext.hpp>

namespace Sirikata {

class ObjectSessionManager;

/** State associated with an Object's session with the space. While
 *  the state can be accessed from other threads, the ObjectSession is
 *  managed and destroyed by the main thread so any data that needs to
 *  be accessed by other threads should be extracted while in the main
 *  strand (e.g. on an ObjectSessionListener::newSession callback).
 */
class SIRIKATA_SPACE_EXPORT ObjectSession {
  public:
    typedef ODPSST::Stream SSTStream;
    typedef SSTStream::Ptr SSTStreamPtr;

    ObjectSession(const ObjectReference& objid)
        : mID(objid),
        mSSTStream(), // set later by ObjectSessionManager
        mSeqNo(new SeqNo())
    {}
    ~ObjectSession()
    {
        // Force closure, there's no way to get data to the object anymore...
        if (mSSTStream) mSSTStream->close(true);
    }

    const ObjectReference& id() const { return mID; }

    SSTStreamPtr getStream() const { return mSSTStream; }

    SeqNoPtr getSeqNoPtr() const { return mSeqNo; }

  private:
    friend class ObjectSessionManager;

    ObjectReference mID;
    SSTStreamPtr mSSTStream;
    // We still use SeqNoPtrs to deal with thread safety -- the seqno
    // is own
    SeqNoPtr mSeqNo;
};

class SIRIKATA_SPACE_EXPORT ObjectSessionListener {
  public:
    virtual void newSession(ObjectSession* session) {}
    virtual void sessionClosed(ObjectSession* session) {}
    virtual ~ObjectSessionListener() {}
};

class SIRIKATA_SPACE_EXPORT ObjectSessionManager : public Provider<ObjectSessionListener*> {
  public:
    ObjectSessionManager(SpaceContext* ctx) {
        ctx->mObjectSessionManager = this;
    }
    virtual ~ObjectSessionManager() {}

    // Owner interface -- adds and removes sessions. Adding is split
    // into two steps. The first adds the session, making it
    // accessible for services that don't need streams. The second
    // completes it
    void addSession(ObjectSession* session) {
        mObjectSessions[session->id()] = session;
    }
    void completeSession(ObjectReference& obj, ObjectSession::SSTStreamPtr s) {
        ObjectSession* session = mObjectSessions[obj];
        session->mSSTStream = s;
        notify(&ObjectSessionListener::newSession, session);
    }
    void removeSession(const ObjectReference& obj) {
        ObjectSessionMap::iterator session_it = mObjectSessions.find(obj);
        if (session_it != mObjectSessions.end()) {
            notify(&ObjectSessionListener::sessionClosed, session_it->second);
            delete session_it->second;
            mObjectSessions.erase(session_it);
        }
    }

    // User interface

    ObjectSession* getSession(const ObjectReference& objid) const {
        ObjectSessionMap::const_iterator it = mObjectSessions.find(objid);
        if (it == mObjectSessions.end()) return NULL;
        return it->second;
    }

  private:
    typedef std::tr1::unordered_map<ObjectReference, ObjectSession*, ObjectReference::Hasher> ObjectSessionMap;
    ObjectSessionMap mObjectSessions;
};

} // namespace Sirikata

#endif //_SIRIKATA_SPACE_OBJECT_SESSION_MANAGER_HPP_

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

#include <sirikata/space/Platform.hpp>
#include <sirikata/core/network/SSTImpl.hpp>
#include <sirikata/core/util/ListenerProvider.hpp>

namespace Sirikata {

class SIRIKATA_SPACE_EXPORT ObjectSession {
  public:
    typedef Stream<SpaceObjectReference> SSTStream;
    typedef SSTStream::Ptr SSTStreamPtr;

    ObjectSession(ObjectReference& objid, SSTStreamPtr strm)
        : mID(objid),
        mSSTStream(strm)
    {}
    ~ObjectSession()
    {
        mSSTStream->close(false);
    }

    const ObjectReference& id() const { return mID; }

    SSTStreamPtr getStream() const { return mSSTStream; }

  private:
    ObjectReference mID;
    SSTStreamPtr mSSTStream;
};

class SIRIKATA_SPACE_EXPORT ObjectSessionListener {
  public:
    virtual void newSession(ObjectSession* session) {}
    virtual void sessionClosed(ObjectSession* session) {}
    virtual ~ObjectSessionListener() {}
};

class SIRIKATA_SPACE_EXPORT ObjectSessionManager : public Provider<ObjectSessionListener*> {
  public:
    virtual ObjectSession* getSession(const ObjectReference& objid) const = 0;
    virtual ~ObjectSessionManager() {}
};

} // namespace Sirikata

#endif //_SIRIKATA_SPACE_OBJECT_SESSION_MANAGER_HPP_

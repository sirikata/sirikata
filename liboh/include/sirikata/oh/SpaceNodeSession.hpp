// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_LIBOH_SPACE_NODE_SESSION_HPP_
#define _SIRIKATA_LIBOH_SPACE_NODE_SESSION_HPP_

#include <sirikata/oh/Platform.hpp>
#include <sirikata/core/util/ListenerProvider.hpp>
#include <sirikata/core/ohdp/SST.hpp>

namespace Sirikata {

class SIRIKATA_OH_EXPORT SpaceNodeSessionListener {
  public:
    virtual ~SpaceNodeSessionListener() {}

    virtual void onSpaceNodeSession(const OHDP::SpaceNodeID& id, OHDPSST::Stream::Ptr sn_stream) {}
    virtual void onSpaceNodeSessionEnded(const OHDP::SpaceNodeID& id) {}
};

typedef Provider<SpaceNodeSessionListener*> SpaceNodeSessionProvider;

/** Small class that acts as a Provider for SpaceNodeSessionListeners, but just
 * forwards events from the real provider (allowing us to provide the
 * SpaceNodeSessionManager before the real provider is created).
 */
class SpaceNodeSessionManager : public SpaceNodeSessionProvider {
  public:
    SpaceNodeSessionManager() {
    }

    // Owner interface
    void fireSpaceNodeSession(const OHDP::SpaceNodeID& id, OHDPSST::Stream::Ptr sn_stream) {
        notify(&SpaceNodeSessionListener::onSpaceNodeSession, id, sn_stream);
        mSNSessions[id] = sn_stream;
    }
    void fireSpaceNodeSessionEnded(const OHDP::SpaceNodeID& id) {
        notify(&SpaceNodeSessionListener::onSpaceNodeSessionEnded, id);
        SpaceNodeSessionMap::const_iterator it = mSNSessions.find(id);
        if (it != mSNSessions.end()) {
            // Force closure of the stream
            if (it->second) it->second->close(true);
            mSNSessions.erase(it);
        }
    }


    // User interface
    OHDPSST::Stream::Ptr getSession(const OHDP::SpaceNodeID& id) {
        SpaceNodeSessionMap::const_iterator it = mSNSessions.find(id);
        if (it == mSNSessions.end()) return OHDPSST::Stream::Ptr();
        return it->second;
    }

  private:
    typedef std::tr1::unordered_map<OHDP::SpaceNodeID, OHDPSST::Stream::Ptr, OHDP::SpaceNodeID::Hasher> SpaceNodeSessionMap;
    SpaceNodeSessionMap mSNSessions;
};

} // namespace Sirikata

#endif //_SIRIKATA_LIBOH_SPACE_NODE_SESSION_HPP_

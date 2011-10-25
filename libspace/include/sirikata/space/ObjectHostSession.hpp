// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_LIBSPACE_OBJECT_HOST_SESSION_HPP_
#define _SIRIKATA_LIBSPACE_OBJECT_HOST_SESSION_HPP_

#include <sirikata/space/Platform.hpp>
#include <sirikata/core/util/ListenerProvider.hpp>
#include <sirikata/core/ohdp/SST.hpp>
#include <sirikata/space/SpaceContext.hpp>

namespace Sirikata {

// These classes are thin wrappers around ObjectHostConnectionManager and it's
// affiliated classes for tracking sessions

class SIRIKATA_SPACE_EXPORT ObjectHostSessionListener {
  public:
    virtual ~ObjectHostSessionListener() {}

    virtual void onObjectHostSession(const OHDP::NodeID& id, OHDPSST::Stream::Ptr oh_stream) {}
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

    void fireObjectHostSession(const OHDP::NodeID& id, OHDPSST::Stream::Ptr oh_stream) {
        notify(&ObjectHostSessionListener::onObjectHostSession, id, oh_stream);
    }
    void fireObjectHostSessionEnded(const OHDP::NodeID& id) {
        notify(&ObjectHostSessionListener::onObjectHostSessionEnded, id);
    }
};

} // namespace Sirikata

#endif //_SIRIKATA_LIBSPACE_OBJECT_HOST_SESSION_HPP_

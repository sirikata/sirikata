// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_LIBOH_OBJECT_NODE_SESSION_HPP_
#define _SIRIKATA_LIBOH_OBJECT_NODE_SESSION_HPP_

#include <sirikata/oh/Platform.hpp>
#include <sirikata/core/util/ListenerProvider.hpp>
#include <sirikata/core/ohdp/Defs.hpp>

namespace Sirikata {

/** Listener for object sessions with individual space servers. */
class SIRIKATA_OH_EXPORT ObjectNodeSessionListener {
  public:
    virtual ~ObjectNodeSessionListener() {}

    /** Invoked when an object begins a session with the given space server
     *  node. Upon invokation, previous sessions should be considered
     *  closed. A null NodeID indicates disconnection from the space.
     */
    virtual void onObjectNodeSession(const SpaceID& space, const ObjectReference& sporef, const OHDP::NodeID& id) {}
};

typedef Provider<ObjectNodeSessionListener*> ObjectNodeSessionProvider;

} // namespace Sirikata

#endif //_SIRIKATA_LIBOH_SPACE_NODE_SESSION_HPP_

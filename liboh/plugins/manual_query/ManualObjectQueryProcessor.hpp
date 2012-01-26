// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_OH_MANUAL_OBJECT_QUERY_PROCESSOR_HPP_
#define _SIRIKATA_OH_MANUAL_OBJECT_QUERY_PROCESSOR_HPP_

#include <sirikata/oh/ObjectQueryProcessor.hpp>
#include <sirikata/oh/ObjectNodeSession.hpp>

#include "OHLocationServiceCache.hpp"
#include "ServerQueryHandler.hpp"

namespace Sirikata {
namespace OH {
namespace Manual {

/** An implementation of ObjectQueryProcessor that manually manages a cut on the
 *  space servers' data structures, creates and manages a local replica of the
 *  servers' data structure, and locally executes queries over that data
 *  structure.
 */
class ManualObjectQueryProcessor :
        public ObjectQueryProcessor,
        public ObjectNodeSessionListener
{
public:
    static ManualObjectQueryProcessor* create(ObjectHostContext* ctx, const String& args);

    ManualObjectQueryProcessor(ObjectHostContext* ctx);
    virtual ~ManualObjectQueryProcessor();

    virtual void start();
    virtual void stop();

    // ObjectNodeSessionListener Interface
    virtual void onObjectNodeSession(const SpaceID& space, const ObjectReference& oref, const OHDP::NodeID& id);

    virtual String connectRequest(HostedObjectPtr ho, const SpaceObjectReference& sporef, const String& query);
    virtual void updateQuery(HostedObjectPtr ho, const SpaceObjectReference& sporef, const String& new_query);

private:

    ObjectHostContext* mContext;

    // Queries registered by objects to be resolved locally and
    // connection state
    struct ObjectState {
        ObjectState()
         : node(OHDP::NodeID::null()),
           query()
        {}

        // Checks if it is safe to destroy this ObjectState
        bool canRemove() const {
            return (node == OHDP::NodeID::null());
        }

        HostedObjectWPtr who;
        OHDP::NodeID node;
        String query;
    };
    typedef std::tr1::unordered_map<SpaceObjectReference, ObjectState, SpaceObjectReference::Hasher> ObjectStateMap;
    ObjectStateMap mObjectState;

    ServerQueryHandler mServerQueryHandler;
};

} // namespace Manual
} // namespace OH
} // namespace Sirikata

#endif //_SIRIKATA_OH_MANUAL_OBJECT_QUERY_PROCESSOR_HPP_

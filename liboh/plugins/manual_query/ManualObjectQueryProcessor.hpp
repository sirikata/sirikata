// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_OH_MANUAL_OBJECT_QUERY_PROCESSOR_HPP_
#define _SIRIKATA_OH_MANUAL_OBJECT_QUERY_PROCESSOR_HPP_

#include <sirikata/oh/ObjectQueryProcessor.hpp>
#include <sirikata/oh/SpaceNodeSession.hpp>
#include <sirikata/oh/ObjectNodeSession.hpp>

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
        public SpaceNodeSessionListener,
        public ObjectNodeSessionListener
{
public:
    static ManualObjectQueryProcessor* create(ObjectHostContext* ctx, const String& args);

    ManualObjectQueryProcessor(ObjectHostContext* ctx);
    virtual ~ManualObjectQueryProcessor();

    virtual void start();
    virtual void stop();

    // SpaceNodeSessionListener Interface
    virtual void onSpaceNodeSession(const OHDP::SpaceNodeID& id, OHDPSST::Stream::Ptr sn_stream);
    virtual void onSpaceNodeSessionEnded(const OHDP::SpaceNodeID& id);

    // ObjectNodeSessionListener Interface
    virtual void onObjectNodeSession(const SpaceID& space, const ObjectReference& sporef, const OHDP::NodeID& id);

    virtual void updateQuery(HostedObjectPtr ho, const SpaceObjectReference& sporef, const String& new_query);

private:

    ObjectHostContext* mContext;
};

} // namespace Manual
} // namespace OH
} // namespace Sirikata

#endif //_SIRIKATA_OH_MANUAL_OBJECT_QUERY_PROCESSOR_HPP_

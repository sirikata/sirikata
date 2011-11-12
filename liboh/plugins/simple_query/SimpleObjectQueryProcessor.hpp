// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_OH_SIMPLE_OBJECT_QUERY_PROCESSOR_HPP_
#define _SIRIKATA_OH_SIMPLE_OBJECT_QUERY_PROCESSOR_HPP_

#include <sirikata/oh/ObjectQueryProcessor.hpp>

namespace Sirikata {
namespace OH {
namespace Simple {

/** A simple implementation of ObjectQueryProcessor: it mostly acts as a pass
 *  through, only managing the coordination with the space server. Query updates
 *  are passed directly through and it only manages listening for new result
 *  streams and parsing the results.
 */
class SimpleObjectQueryProcessor : public ObjectQueryProcessor {
public:
    static SimpleObjectQueryProcessor* create(ObjectHostContext* ctx, const String& args);

    SimpleObjectQueryProcessor(ObjectHostContext* ctx);
    virtual ~SimpleObjectQueryProcessor();

    virtual void start();
    virtual void stop();
    virtual void presenceConnectedStream(HostedObjectPtr ho, const SpaceObjectReference& sporef, HostedObject::SSTStreamPtr strm);
    virtual void updateQuery(HostedObjectPtr ho, const SpaceObjectReference& sporef, const String& new_query);

private:
    // Proximity
    void handleProximitySubstream(const HostedObjectWPtr &weakSelf, const SpaceObjectReference& spaceobj, int err, SSTStreamPtr s);
    void handleProximitySubstreamRead(const HostedObjectWPtr &weakSelf, const SpaceObjectReference& spaceobj, SSTStreamPtr s, String* prevdata, uint8* buffer, int length);
    bool handleProximityMessage(HostedObjectPtr self, const SpaceObjectReference& spaceobj, const std::string& payload);

    // Location
    // Handlers for substreams for space-managed updates
    void handleLocationSubstream(const HostedObjectWPtr &weakSelf, const SpaceObjectReference& spaceobj, int err, SSTStreamPtr s);
    // Handlers for substream read events for space-managed updates
    void handleLocationSubstreamRead(const HostedObjectWPtr &weakSelf, const SpaceObjectReference& spaceobj, SSTStreamPtr s, std::stringstream* prevdata, uint8* buffer, int length);
    bool handleLocationMessage(const HostedObjectPtr& self, const SpaceObjectReference& spaceobj, const std::string& paylod);


    ObjectHostContext* mContext;
};

} // namespace Simple
} // namespace OH
} // namespace Sirikata

#endif //_SIRIKATA_OH_SIMPLE_OBJECT_QUERY_PROCESSOR_HPP_

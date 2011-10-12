// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_OH_OBJECT_QUERY_PROCESSOR_HPP_
#define _SIRIKATA_OH_OBJECT_QUERY_PROCESSOR_HPP_

#include <sirikata/oh/Platform.hpp>
#include <sirikata/oh/HostedObject.hpp>
#include <sirikata/core/service/Service.hpp>
#include <sirikata/core/util/Factory.hpp>

namespace Sirikata {

namespace Protocol {
namespace Prox {
class ProximityResults;
}
}

namespace OH {

/** ObjectQueryProcessor is the interface for a geometric (proximity) query
 *  service for objects in an object host. The query format is opaque: different
 *  implementations may support different query formats. Results are provided
 *  directly to the HostedObject in the form of
 *  Protocol::Prox::ProximityResults, as sets of atomic additions and
 *  removals. An implementation may generate results in different ways -- simple
 *  versions might just forward requests to the server and manage receiving
 *  results whereas more complicated versions might process queries locally.
 */
class SIRIKATA_OH_EXPORT ObjectQueryProcessor : public Service {
  public:
    typedef HostedObject::SSTStreamPtr SSTStreamPtr;
    typedef HostedObject::SSTConnectionPtr SSTConnectionPtr;

    virtual ~ObjectQueryProcessor();

    // Connection/disconnection to track unrequested

    /** Invoked by the ObjectHost when an object's presence is
     *  connected to the space. While it has received an
     *  acknowledgement, the connection may not fully be established
     *  (e.g. streams may not have finished connecting).
     */
    virtual void presenceConnected(HostedObjectPtr ho, const SpaceObjectReference& sporef);
    /** Invoked by the ObjectHost when an object's presence is
     *  connected to the space and the underlying stream has been
     *  fully established.
     */
    virtual void presenceConnectedStream(HostedObjectPtr ho, const SpaceObjectReference& sporef, SSTStreamPtr strm);
    /** Invoked by the ObjectHost when an object's presence has been
     *  disconnected from the space.
     */
    virtual void presenceDisconnected(HostedObjectPtr ho, const SpaceObjectReference& sporef);


    /** Request an update to the query parameters for the specified presence.
     *  \param ho the HostedObject requesting the update
     *  \param sporef the ID of the presence to update a query for
     *  \param new_query the encoded, updated query
     */
    virtual void updateQuery(HostedObjectPtr ho, const SpaceObjectReference& sporef, const String& new_query) = 0;

  protected:
    /** Helper method for implementations which delivers results to the
     *  HostedObject.
     *  \param ho the HostedObject requesting the update
     *  \param sporef the ID of the presence that registered the query
     *  \param results the new results to be returned
     */
    void deliverResults(HostedObjectPtr ho, const SpaceObjectReference& sporef, const Sirikata::Protocol::Prox::ProximityResults& results);
};


class SIRIKATA_OH_EXPORT ObjectQueryProcessorFactory
    : public AutoSingleton<ObjectQueryProcessorFactory>,
      public Factory2<ObjectQueryProcessor*, ObjectHostContext*, const String&>
{
public:
    static ObjectQueryProcessorFactory& getSingleton();
    static void destroy();
};

} // namespace OH
} // namespace Sirikata

#endif //_SIRIKATA_OH_OBJECT_QUERY_PROCESSOR_HPP_

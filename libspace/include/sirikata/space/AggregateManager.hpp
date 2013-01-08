// Copyright (c) 2013 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_SPACE_AGGREGATE_MANAGER_HPP_
#define _SIRIKATA_SPACE_AGGREGATE_MANAGER_HPP_

#include <sirikata/core/util/Factory.hpp>
#include <sirikata/space/LocationService.hpp>
#include <sirikata/core/transfer/TransferMediator.hpp>

namespace Sirikata {

class SIRIKATA_SPACE_EXPORT AggregateManager : public LocationServiceListener {

  public:
    virtual void addAggregate(const UUID& uuid) = 0;
    virtual void removeAggregate(const UUID& uuid) = 0;
    virtual void addChild(const UUID& uuid, const UUID& child_uuid) = 0;
    virtual void removeChild(const UUID& uuid, const UUID& child_uuid) = 0;
    virtual void aggregateObserved(const UUID& objid, uint32 nobservers, uint32 nchildren) = 0;
    virtual void generateAggregateMesh(const UUID& uuid, const Duration& delayFor = Duration::milliseconds(1.0f) ) = 0;
};

class SIRIKATA_SPACE_EXPORT AggregateManagerFactory
    : public AutoSingleton<AggregateManagerFactory>,
      public Factory3<AggregateManager*, LocationService* /*loc*/, Transfer::OAuthParamsPtr /*oauth*/, const String& /*username*/>
{
  public:
    static AggregateManagerFactory& getSingleton();
    static void destroy();
}; // class AggregateManagerFactory


} // namespace Sirikata

#endif //_SIRIKATA_SPACE_AGGREGATE_MANAGER_HPP_

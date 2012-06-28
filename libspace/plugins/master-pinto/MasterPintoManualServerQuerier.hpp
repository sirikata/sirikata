// Copyright (c) 2012 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_LIBSPACE_MASTER_PINTO_MANUAL_SERVER_QUERIER_HPP_
#define _SIRIKATA_LIBSPACE_MASTER_PINTO_MANUAL_SERVER_QUERIER_HPP_

#include "MasterPintoServerQuerierBase.hpp"

namespace Sirikata {

/** Implementation of client for centralized server for space server discovery,
 *  using manual tree traversal and replication.
 */
class MasterPintoManualServerQuerier : public MasterPintoServerQuerierBase {
public:
    MasterPintoManualServerQuerier(SpaceContext* ctx, const String& params);
    virtual ~MasterPintoManualServerQuerier();

    // PintoServerQuerier Interface
    virtual void updateQuery(const String& update);

private:

    // Overrides from base class
    virtual void onConnected();

    virtual void onPintoData(Sirikata::Protocol::MasterPinto::PintoResponse& msg);

    // If connected and any properties are marked as dirty, tries to
    // send an update to the server
    void updatePintoQuery();


    SolidAngle mAggregateQuery;
    uint32 mAggregateQueryMaxResults;
    bool mAggregateQueryDirty;
}; // MasterPintoManualServerQuerier

} // namespace Sirikata

#endif //_SIRIKATA_LIBSPACE_MASTER_PINTO_MANUAL_SERVER_QUERIER_HPP_

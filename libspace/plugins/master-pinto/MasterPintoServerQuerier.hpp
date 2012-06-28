// Copyright (c) 2012 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_LIBSPACE_MASTER_PINTO_SERVER_QUERIER_HPP_
#define _SIRIKATA_LIBSPACE_MASTER_PINTO_SERVER_QUERIER_HPP_

#include "MasterPintoServerQuerierBase.hpp"

namespace Sirikata {

/** Implementation of client for centralized server for space server discovery,
 *  using aggregated solid angle queries.
 */
class MasterPintoServerQuerier : public MasterPintoServerQuerierBase {
public:
    MasterPintoServerQuerier(SpaceContext* ctx, const String& params);
    virtual ~MasterPintoServerQuerier();

    // PintoServerQuerier Interface
    virtual void updateQuery(const String& update);

private:

    // Overrides from base class
    virtual void onConnected();

    virtual void onPintoData(const String& data);

    // If connected and any properties are marked as dirty, tries to
    // send an update to the server
    void updatePintoQuery();


    SolidAngle mAggregateQuery;
    uint32 mAggregateQueryMaxResults;
    bool mAggregateQueryDirty;
}; // MasterPintoServerQuerier

} // namespace Sirikata

#endif //_SIRIKATA_LIBSPACE_MASTER_PINTO_SERVER_QUERIER_HPP_

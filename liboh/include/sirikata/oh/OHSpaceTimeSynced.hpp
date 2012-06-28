// Copyright (c) 2012 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_LIBOH_OH_SPACE_TIME_SYNCED_HPP_
#define _SIRIKATA_LIBOH_OH_SPACE_TIME_SYNCED_HPP_

#include <sirikata/pintoloc/TimeSynced.hpp>
#include <sirikata/oh/ObjectHost.hpp>

namespace Sirikata {

/** Implementation of TimeSynced that uses an ObjectHost for the sync. You need
 * to specify which space to sync with.
 */
class SIRIKATA_OH_EXPORT OHSpaceTimeSynced : public TimeSynced {
  public:
    OHSpaceTimeSynced(ObjectHost* oh, const SpaceID& space)
        : mOH(oh), mSpace(space)
    {}
    virtual ~OHSpaceTimeSynced() {}

    virtual Time localTime(const Time& t) const {
        return mOH->localTime(mSpace, t);
    }
    virtual Time remoteTime(const Time& t) const {
        return mOH->spaceTime(mSpace, t);
    }
  private:
    ObjectHost* mOH;
    SpaceID mSpace;
}; // class OHSpaceTimeSynced

} // namespace Sirikata

#endif //_SIRIKATA_LIBOH_OH_SPACE_TIME_SYNCED_HPP_

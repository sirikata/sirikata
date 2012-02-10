// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include <sirikata/oh/ProtocolLocUpdate.hpp>
#include <sirikata/oh/ObjectHost.hpp>

namespace Sirikata {

TimedMotionVector3f LocProtocolLocUpdate::location() const {
    Sirikata::Protocol::TimedMotionVector update_loc = mUpdate.location();
    return TimedMotionVector3f(
        mOH->localTime(mSpace, update_loc.t()),
        MotionVector3f(update_loc.position(), update_loc.velocity())
    );
}

TimedMotionQuaternion LocProtocolLocUpdate::orientation() const {
    Sirikata::Protocol::TimedMotionQuaternion update_orient = mUpdate.orientation();
    return TimedMotionQuaternion(
        mOH->localTime(mSpace, update_orient.t()),
        MotionQuaternion(update_orient.position(), update_orient.velocity())
    );
}


TimedMotionVector3f ProxProtocolLocUpdate::location() const {
    Sirikata::Protocol::TimedMotionVector update_loc = mUpdate.location();
    return TimedMotionVector3f(
        mOH->localTime(mSpace, update_loc.t()),
        MotionVector3f(update_loc.position(), update_loc.velocity())
    );
}

TimedMotionQuaternion ProxProtocolLocUpdate::orientation() const {
    Sirikata::Protocol::TimedMotionQuaternion update_orient = mUpdate.orientation();
    return TimedMotionQuaternion(
        mOH->localTime(mSpace, update_orient.t()),
        MotionQuaternion(update_orient.position(), update_orient.velocity())
    );
}


} // namespace Sirikata

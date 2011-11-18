// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.


#include <sirikata/oh/Platform.hpp>
#include <sirikata/oh/LocUpdate.hpp>
#include <sirikata/oh/ObjectHost.hpp>
#include <sirikata/oh/HostedObject.hpp>

namespace Sirikata {

TimedMotionVector3f LocUpdate::locationWithLocalTime(ObjectHost* oh, const SpaceID& from_space) const {
    TimedMotionVector3f orig = location();
    return TimedMotionVector3f(oh->localTime(from_space, orig.updateTime()), orig.value());
}

TimedMotionVector3f LocUpdate::locationWithLocalTime(HostedObject* ho, const SpaceID& from_space) const {
    return locationWithLocalTime(ho->getObjectHost(), from_space);
}

TimedMotionVector3f LocUpdate::locationWithLocalTime(HostedObjectPtr ho, const SpaceID& from_space) const {
    return locationWithLocalTime(ho->getObjectHost(), from_space);
}


TimedMotionQuaternion LocUpdate::orientationWithLocalTime(ObjectHost* oh, const SpaceID& from_space) const {
    TimedMotionQuaternion orig = orientation();
    return TimedMotionQuaternion(oh->localTime(from_space, orig.updateTime()), orig.value());
}

TimedMotionQuaternion LocUpdate::orientationWithLocalTime(HostedObject* ho, const SpaceID& from_space) const {
    return orientationWithLocalTime(ho->getObjectHost(), from_space);
}

TimedMotionQuaternion LocUpdate::orientationWithLocalTime(HostedObjectPtr ho, const SpaceID& from_space) const {
    return orientationWithLocalTime(ho->getObjectHost(), from_space);
}

} // namespace

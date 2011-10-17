// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include <sirikata/core/util/Standard.hh>
#include <sirikata/core/xdp/Defs.hpp>

namespace Sirikata {
namespace XDP {
// PortID

#define NULL_PORT_ID 0
#define ANY_PORT_ID 0xFFFFFF

PortID::PortID()
 : mValue(NULL_PORT_ID)
{
}

PortID::PortID(uint32 rhs)
 : mValue(rhs)
{
}

PortID::PortID(const PortID& rhs)
 : mValue(rhs.mValue)
{
}

const PortID& PortID::null() {
    static PortID null_port(NULL_PORT_ID);
    return null_port;
}

const PortID& PortID::any() {
    static PortID any_port(ANY_PORT_ID);
    return any_port;
}

PortID& PortID::operator=(const PortID& rhs) {
    mValue = rhs.mValue;
    return *this;
}

PortID& PortID::operator=(uint32 rhs) {
    mValue = rhs;
    return *this;
}

PortID::operator uint32() const {
    return mValue;
}

bool PortID::operator==(const PortID& rhs) const {
    return mValue == rhs.mValue;
}


bool PortID::operator!=(const PortID& rhs) const {
    return mValue != rhs.mValue;
}

bool PortID::operator>(const PortID& rhs) const {
    return mValue > rhs.mValue;
}

bool PortID::operator>=(const PortID& rhs) const {
    return mValue >= rhs.mValue;
}

bool PortID::operator<(const PortID& rhs) const {
    return mValue < rhs.mValue;
}

bool PortID::operator<=(const PortID& rhs) const {
    return mValue <= rhs.mValue;
}

bool PortID::matches(const PortID& rhs) const {
    return (
        mValue == rhs.mValue ||
        mValue == ANY_PORT_ID ||
        rhs.mValue == ANY_PORT_ID);
}

} // namespace XDP
} // namespace Sirikata

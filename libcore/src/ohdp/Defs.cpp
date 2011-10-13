// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include <sirikata/core/util/Standard.hh>
#include <sirikata/core/ohdp/Defs.hpp>

namespace Sirikata {
namespace OHDP {

#define NULL_NODE_ID 0
#define ANY_NODE_ID 0xFFFFFF

NodeID::NodeID()
 : mValue(NULL_NODE_ID)
{
}

NodeID::NodeID(uint32 rhs)
 : mValue(rhs)
{
}

NodeID::NodeID(const NodeID& rhs)
 : mValue(rhs.mValue)
{
}

const NodeID& NodeID::null() {
    static NodeID null_port(NULL_NODE_ID);
    return null_port;
}

const NodeID& NodeID::any() {
    static NodeID any_port(ANY_NODE_ID);
    return any_port;
}

NodeID& NodeID::operator=(const NodeID& rhs) {
    mValue = rhs.mValue;
    return *this;
}

NodeID& NodeID::operator=(uint32 rhs) {
    mValue = rhs;
    return *this;
}

NodeID::operator uint32() const {
    return mValue;
}

bool NodeID::operator==(const NodeID& rhs) const {
    return mValue == rhs.mValue;
}


bool NodeID::operator!=(const NodeID& rhs) const {
    return mValue != rhs.mValue;
}

bool NodeID::operator>(const NodeID& rhs) const {
    return mValue > rhs.mValue;
}

bool NodeID::operator>=(const NodeID& rhs) const {
    return mValue >= rhs.mValue;
}

bool NodeID::operator<(const NodeID& rhs) const {
    return mValue < rhs.mValue;
}

bool NodeID::operator<=(const NodeID& rhs) const {
    return mValue <= rhs.mValue;
}

bool NodeID::matches(const NodeID& rhs) const {
    return (
        mValue == rhs.mValue ||
        mValue == ANY_NODE_ID ||
        rhs.mValue == ANY_NODE_ID);
}

std::ostream& operator<<(std::ostream& os, const NodeID& rhs) {
    os << (uint32)rhs;
    return os;
}

} // namespace OHDP
} // namespace Sirikata

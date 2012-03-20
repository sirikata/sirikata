// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include <sirikata/core/util/Standard.hh>
#include <sirikata/core/ohdp/Defs.hpp>
#include <boost/functional/hash.hpp>
#include <boost/lexical_cast.hpp>

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

NodeID::NodeID(const String& rhs) {
    try {
        mValue = boost::lexical_cast<uint32>(rhs);
    }
    catch (boost::bad_lexical_cast& blc) {
        mValue = NULL_NODE_ID;
    }
}

const NodeID& NodeID::null() {
    static NodeID null_port(NULL_NODE_ID);
    return null_port;
}

const NodeID& NodeID::self() {
    return null();
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

String NodeID::toString() const {
    return boost::lexical_cast<String>(mValue);
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



SpaceNodeID::SpaceNodeID()
 : mSpace(SpaceID::null()),
   mNode(NodeID::null())
{
}

SpaceNodeID::SpaceNodeID(const SpaceID& s, const NodeID& n)
 : mSpace(s),
   mNode(n)
{
}

SpaceNodeID::SpaceNodeID(const String& rhs)
 : mSpace(SpaceID::null()),
   mNode(NodeID::null())
{
    std::size_t split = rhs.find(':');
    if (split == String::npos) return;

    String space_part = rhs.substr(0, split);
    SpaceID sid(space_part);
    String node_part = rhs.substr(split+1);
    NodeID nid(node_part);

    if (sid != SpaceID::null() &&
        nid != NodeID::null()) {
        mSpace = sid;
        mNode = nid;
    }
}

const SpaceNodeID& SpaceNodeID::null() {
    static SpaceNodeID n(SpaceID::null(), NodeID::null());
    return n;
}

bool SpaceNodeID::operator==(const SpaceNodeID& rhs) const {
    return (mSpace == rhs.mSpace && mNode == rhs.mNode);
}

bool SpaceNodeID::operator<(const SpaceNodeID& rhs) const {
    return (
        mSpace < rhs.mSpace ||
        (mSpace == rhs.mSpace && mNode < rhs.mNode)
    );
}

String SpaceNodeID::toString() const {
    return mSpace.toString() + ":" + mNode.toString();
}

size_t SpaceNodeID::Hasher::operator()(const SpaceNodeID& p) const {
    size_t seed = 0;
    boost::hash_combine(seed, p.mSpace.hash());
    boost::hash_combine(seed, p.mNode.hash());
    return seed;
}

std::ostream& operator<<(std::ostream& os, const SpaceNodeID& rhs) {
    os << rhs.space() << ":" << rhs.node();
    return os;
}


} // namespace OHDP
} // namespace Sirikata

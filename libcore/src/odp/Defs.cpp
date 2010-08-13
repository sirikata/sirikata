/*  Sirikata
 *  Defs.cpp
 *
 *  Copyright (c) 2010, Ewen Cheslack-Postava
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Sirikata nor the names of its contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sirikata/core/util/Standard.hh>
#include <sirikata/core/odp/Defs.hpp>
#include <sirikata/core/odp/Exceptions.hpp>

namespace Sirikata {
namespace ODP {
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


// Endpoint

Endpoint::Endpoint(const SpaceID& space, const ObjectReference& obj, const PortID& port)
 : mSpace(space),
   mObject(obj),
   mPort(port)
{
}

Endpoint::Endpoint(const SpaceObjectReference& space_obj, const PortID& port)
 : mSpace(space_obj.space()),
   mObject(space_obj.object()),
   mPort(port)
{
}

bool Endpoint::operator==(const Endpoint& rhs) const {
    return (
        mSpace == rhs.mSpace &&
        mObject == rhs.mObject &&
        mPort == rhs.mPort
    );
}

bool Endpoint::operator!=(const Endpoint& rhs) const {
    return !(*this == rhs);
}

bool Endpoint::operator>(const Endpoint& rhs) const {
    return (
        mSpace > rhs.mSpace || (
            mSpace == rhs.mSpace && (
                mObject > rhs.mObject || (
                    mObject == rhs.mObject && (
                        mPort > rhs.mPort
                    )
                )
            )
        )
    );
}

bool Endpoint::operator>=(const Endpoint& rhs) const {
    return !(*this < rhs);
}

bool Endpoint::operator<(const Endpoint& rhs) const {
    return (
        mSpace < rhs.mSpace || (
            mSpace == rhs.mSpace && (
                mObject < rhs.mObject || (
                    mObject == rhs.mObject && (
                        mPort < rhs.mPort
                    )
                )
            )
        )
    );
}

bool Endpoint::operator<=(const Endpoint& rhs) const {
    return !(*this > rhs);
}

bool Endpoint::matches(const Endpoint& rhs) const {
    return (
        mSpace.matches(rhs.mSpace) &&
        mObject.matches(rhs.mObject) &&
        mPort.matches(rhs.mPort)
    );
}

const Endpoint& Endpoint::null() {
    static Endpoint null_ep(SpaceID::null(), ObjectReference::null(), PortID::null());
    return null_ep;
}

const Endpoint& Endpoint::any() {
    static Endpoint any_ep(SpaceID::any(), ObjectReference::any(), PortID::any());
    return any_ep;
}

const SpaceID& Endpoint::space() const {
    return mSpace;
}

const ObjectReference& Endpoint::object() const {
    return mObject;
}

const PortID& Endpoint::port() const {
    return mPort;
}

String Endpoint::toString() const {
    std::ostringstream ss;
    ss << (*this);
    return ss.str();
}

std::ostream& operator<<(std::ostream& os, const Sirikata::ODP::Endpoint& ep) {
    os << ep.space() << ":" << ep.object() << ":" << ep.port();
    return os;
}

} // namespace ODP
} // namespace Sirikata

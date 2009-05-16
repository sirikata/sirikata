/*  Sirikata Object Host -- Proximity Connection Class
 *  ProximityConnection.hpp
 *
 *  Copyright (c) 2009, Daniel Reiter Horn
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

namespace Sirikata { namespace Network {
class Stream;
class Address;
class IOService;
} }
namespace Sirikata { namespace Protocol {
class IMessage;

} }

namespace Sirikata { namespace Proximity {
class ProximitySystem;

class SIRIKATA_PROXIMITY_EXPORT SingleStreamProximityConnection :public ProximityConnection{
    ProximitySystem *mParent;
    std::tr1::shared_ptr<Network::Stream> mConnectionStream;
    typedef std::tr1::unordered_map<ObjectReference,Network::Stream*,ObjectReference::Hasher> ObjectStreamMap;
    ObjectStreamMap mObjectStreams;
    
public:
    static ProximityConnection* create(Network::IOService*, const String&);
    void streamDisconnected();
    SingleStreamProximityConnection(const Network::Address&addy, Network::IOService&);
    void setParent(ProximitySystem*parent){mParent=parent;}
    ~SingleStreamProximityConnection();
    void constructObjectStream(const ObjectReference&obc);
    void deleteObjectStream(const ObjectReference&obc);
    void send(const ObjectReference&,
              const Protocol::IMessage&,
              const void*optionalSerializedMessage,
              const size_t optionalSerializedMessageSize);
};

} }

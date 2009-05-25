/*  Sirikata Proximity Management -- Introduction Services
 *  ProximitySystem.cpp
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

#include "proximity/Platform.hpp"
#include "Proximity_Sirikata.pbj.hpp"
#include "util/RoutableMessageHeader.hpp"
#include "proximity/ProximitySystem.hpp"
#include "network/Stream.hpp"
//#include "proximity/ObjectSpaceBridgeProximitySystem.hpp"
//for testing only#include "proximity/BridgeProximitySystem.hpp"
namespace Sirikata { namespace Proximity {
ProximitySystem::~ProximitySystem(){}
void ProximitySystem::defaultProximityCallback(Network::Stream*strm, const RoutableMessageHeader&hdr,const Sirikata::Protocol::IMessageBody&msg){
    assert(strm);
    std::string data;
    hdr.SerializeToString(&data);
    msg.AppendToString(&data);
    strm->send(MemoryReference(data),Network::ReliableOrdered);
}
void ProximitySystem::defaultNoAddressProximityCallback(Network::Stream*strm, const RoutableMessageHeader&const_hdr,const Sirikata::Protocol::IMessageBody&msg){
    assert(strm);
    RoutableMessageHeader h(const_hdr); 
    std::string data;
    h.clear_destination_object();
    h.clear_destination_space();
    h.clear_source_object();
    h.clear_source_space();
    h.SerializeToString(&data);
    msg.AppendToString(&data);
    strm->send(MemoryReference(data),Network::ReliableOrdered);
}
void ProximitySystem::defaultNoDestinationAddressProximityCallback(Network::Stream*strm, const RoutableMessageHeader&const_hdr,const Sirikata::Protocol::IMessageBody&msg){
    assert(strm);
    RoutableMessageHeader h(const_hdr); 
    std::string data;
    h.clear_destination_object();
    h.clear_destination_space();
    h.SerializeToString(&data);
    msg.AppendToString(&data);
    strm->send(MemoryReference(data),Network::ReliableOrdered);
}


/*
bool ProximitySystem::proximityRelevantMessageName(const String&msg){
    return msg=="ObjLoc"||msg=="ProxCall"||msg=="NewProxQuery"||msg=="DelProxQuery"||msg=="RetObj"||msg=="DelObj";
}
bool ProximitySystem::proximityRelevantMessage(const Sirikata::Protocol::IMessage&msg){
    int num_messages=msg.message_names_size();
    for (int i=0;i<num_messages;++i) {
        if (proximityRelevantMessageName(msg.message_names(i)))
            return true;
    }
    return false;
}
*/


} }

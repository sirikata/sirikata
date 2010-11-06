/*  Sirikata
 *  SSTImpl.cpp
 *
 *  Copyright (c) 2009, Tahir Azim.
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

#include <sirikata/core/network/SSTImpl.hpp>

#include <sirikata/core/network/Address.hpp>

#include <sirikata/core/network/Address4.hpp>
#include <sirikata/core/util/Random.hpp>

#include <boost/bind.hpp>


//Assume we have a send(void* data, int len) function and a handleRead(void*) function
//

namespace Sirikata {

/*
typedef BaseDatagramLayer<Sirikata::UUID> UUIDBaseDatagramLayer;
typedef std::tr1::shared_ptr<UUIDBaseDatagramLayer> UUIDBaseDatagramLayerPtr;

template <>  std::map<Sirikata::UUID, UUIDBaseDatagramLayerPtr> UUIDBaseDatagramLayer::sDatagramLayerMap = std::map<Sirikata::UUID, UUIDBaseDatagramLayerPtr> ();

template <> std::map<EndPoint<Sirikata::UUID>  , boost::shared_ptr< Connection<Sirikata::UUID> > > Connection<Sirikata::UUID>::sConnectionMap = std::map<EndPoint<Sirikata::UUID>  , boost::shared_ptr< Connection<Sirikata::UUID> > > ();


template <> std::map<EndPoint<Sirikata::UUID>  , ConnectionReturnCallbackFunction > Connection<Sirikata::UUID>::sConnectionReturnCallbackMap = std::map<EndPoint<Sirikata::UUID>  , ConnectionReturnCallbackFunction > ();

template <> std::map<EndPoint<Sirikata::UUID>  , StreamReturnCallbackFunction > Connection<Sirikata::UUID>::sListeningConnectionsCallbackMap = std::map<EndPoint<Sirikata::UUID>  , StreamReturnCallbackFunction > ();

template <> std::bitset<65536> Connection<Sirikata::UUID>::sAvailableChannels = std::bitset<65536> ();

template <> std::map<EndPoint<Sirikata::UUID>  , StreamReturnCallbackFunction > Stream<Sirikata::UUID>::mStreamReturnCallbackMap = std::map<EndPoint<Sirikata::UUID>  , StreamReturnCallbackFunction > ();

template <> Mutex Connection<Sirikata::UUID>::sStaticMembersLock = Mutex();
*/

typedef BaseDatagramLayer<SpaceObjectReference> SpaceObjectReferenceBaseDatagramLayer;
typedef std::tr1::shared_ptr<SpaceObjectReferenceBaseDatagramLayer> SpaceObjectReferenceBaseDatagramLayerPtr;

template <>  std::map<SpaceObjectReference, SpaceObjectReferenceBaseDatagramLayerPtr> SpaceObjectReferenceBaseDatagramLayer::sDatagramLayerMap = std::map<SpaceObjectReference, SpaceObjectReferenceBaseDatagramLayerPtr> ();

template <> std::map<EndPoint<SpaceObjectReference>  , std::tr1::shared_ptr< Connection<SpaceObjectReference> > > Connection<SpaceObjectReference>::sConnectionMap = std::map<EndPoint<SpaceObjectReference>  , std::tr1::shared_ptr< Connection<SpaceObjectReference> > > ();


template <> std::map<EndPoint<SpaceObjectReference>  , ConnectionReturnCallbackFunction > Connection<SpaceObjectReference>::sConnectionReturnCallbackMap = std::map<EndPoint<SpaceObjectReference>  , ConnectionReturnCallbackFunction > ();

template <> std::map<EndPoint<SpaceObjectReference>  , StreamReturnCallbackFunction > Connection<SpaceObjectReference>::sListeningConnectionsCallbackMap = std::map<EndPoint<SpaceObjectReference>  , StreamReturnCallbackFunction > ();

template <> std::bitset<65536> Connection<SpaceObjectReference>::sAvailableChannels = std::bitset<65536> ();

template <> std::map<EndPoint<SpaceObjectReference>  , StreamReturnCallbackFunction > Stream<SpaceObjectReference>::mStreamReturnCallbackMap = std::map<EndPoint<SpaceObjectReference>  , StreamReturnCallbackFunction > ();
template <> Mutex Connection<SpaceObjectReference>::sStaticMembersLock = Mutex();

}

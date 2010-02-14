/*  cbr
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
 *  * Neither the name of cbr nor the names of its contributors may
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

#include "SSTImpl.hpp"


#include <sirikata/network/Address.hpp>
#include "Address4.hpp"




#include "Random.hpp"

#include <boost/bind.hpp>


//Assume we have a send(void* data, int len) function and a handleRead(void*) function
//

namespace CBR {

template <>  std::map<Sirikata::UUID, boost::shared_ptr< BaseDatagramLayer<Sirikata::UUID> > > BaseDatagramLayer<Sirikata::UUID>::mDatagramLayerMap = std::map<Sirikata::UUID, boost::shared_ptr< BaseDatagramLayer<Sirikata::UUID> > > ();

template <> std::map<EndPoint<Sirikata::UUID>  , boost::shared_ptr< Connection<Sirikata::UUID> > > Connection<Sirikata::UUID>::mConnectionMap = std::map<EndPoint<Sirikata::UUID>  , boost::shared_ptr< Connection<Sirikata::UUID> > > ();

    
template <> std::map<EndPoint<Sirikata::UUID>  , ConnectionReturnCallbackFunction > Connection<Sirikata::UUID>::mConnectionReturnCallbackMap = std::map<EndPoint<Sirikata::UUID>  , ConnectionReturnCallbackFunction > ();

template <> std::map<EndPoint<Sirikata::UUID>  , StreamReturnCallbackFunction > Connection<Sirikata::UUID>::mListeningConnectionsCallbackMap = std::map<EndPoint<Sirikata::UUID>  , StreamReturnCallbackFunction > ();

template <> std::bitset<65536> Connection<Sirikata::UUID>::mAvailableChannels = std::bitset<65536> ();
template <> uint16 Connection<Sirikata::UUID>::mLastAssignedPort = 65530;


template <> std::map<EndPoint<Sirikata::UUID>  , StreamReturnCallbackFunction > Stream<Sirikata::UUID>::mStreamReturnCallbackMap = std::map<EndPoint<Sirikata::UUID>  , StreamReturnCallbackFunction > ();

template <> Mutex Stream<Sirikata::UUID>::mStreamCreationMutex = Mutex();

template <> Mutex Connection<Sirikata::UUID>::mStaticMembersLock = Mutex();

}

/*  Sirikata liboh -- Object Host
 *  ObjectHost.cpp
 *
 *  Copyright (c) 2009, Ewen Cheslack-Postava
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

#include <oh/Platform.hpp>
#include <oh/ObjectHost.hpp>
#include <boost/thread.hpp>
#include <network/IOServiceFactory.hpp>
#include "graphics/GraphicsObject.hpp"
#include "oh/TopLevelSpaceConnection.hpp"
#include "oh/SpaceConnection.hpp"

namespace Sirikata {

ObjectHost::ObjectHost(SpaceIDMap *spaceMap, Network::IOService *ioServ) {
    mSpaceIDMap = spaceMap;
    mSpaceConnectionIO=ioServ;
}

ObjectHost::~ObjectHost() {
}

///This method checks if the message is destined for any named mServices. If not, it gives it to mRouter
void ObjectHost::processMessage(const RoutableMessageHeader&header,
                                MemoryReference message_body) {

}
namespace{
    boost::recursive_mutex gSpaceConnectionMapLock;
}
void ObjectHost::insertAddressMapping(const Network::Address&addy, const std::tr1::weak_ptr<TopLevelSpaceConnection>&val){
    boost::recursive_mutex::scoped_lock uniqMap(gSpaceConnectionMapLock);
    mAddressConnections[addy]=val;
}
std::tr1::shared_ptr<TopLevelSpaceConnection> ObjectHost::connectToSpace(const SpaceID&id){
    std::tr1::shared_ptr<TopLevelSpaceConnection> retval;
    {
        boost::recursive_mutex::scoped_lock uniqMap(gSpaceConnectionMapLock);
        SpaceConnectionMap::iterator where=mSpaceConnections.find(id);
        if ((where==mSpaceConnections.end())||((retval=where->second.lock())==NULL)) {
            std::tr1::shared_ptr<TopLevelSpaceConnection> temp(new TopLevelSpaceConnection(mSpaceConnectionIO));
            temp->connect(temp,this,id);//inserts into mSpaceConnections and eventuallly mAddressConnections
            retval = temp;
            if (where==mSpaceConnections.end()) {
                mSpaceConnections.insert(SpaceConnectionMap::value_type(id,retval));
            }else {
                SILOG(oh,warning,"Null spaceid->connection mapping");
                where->second=retval;
            }
        }
    }
    return retval;
}


std::tr1::shared_ptr<TopLevelSpaceConnection> ObjectHost::connectToSpaceAddress(const SpaceID&id, const Network::Address&addy){
    std::tr1::shared_ptr<TopLevelSpaceConnection> retval;
    {
        boost::recursive_mutex::scoped_lock uniqMap(gSpaceConnectionMapLock);
        AddressConnectionMap::iterator where=mAddressConnections.find(addy);
        if ((where==mAddressConnections.end())||(!(retval=where->second.lock()))) {
            std::tr1::shared_ptr<TopLevelSpaceConnection> temp(new TopLevelSpaceConnection(mSpaceConnectionIO));
            temp->connect(temp,this,id,addy);//inserts into mSpaceConnections and eventuallly mAddressConnections
            retval = temp;
            if (where==mAddressConnections.end()) {
                mAddressConnections[addy]=temp;
            }else {
                SILOG(oh,warning,"Null spaceid->connection mapping");
                where->second=retval;
            }
            mSpaceConnections.insert(SpaceConnectionMap::value_type(id,retval));
        }
    }
    return retval;
}


void ObjectHost::removeTopLevelSpaceConnection(const SpaceID&id, const Network::Address& addy,const TopLevelSpaceConnection*example){
    boost::recursive_mutex::scoped_lock uniqMap(gSpaceConnectionMapLock);    
    {
        SpaceConnectionMap::iterator where=mSpaceConnections.find(id);
        for(;where!=mSpaceConnections.end()&&where->first==id;++where) {
            std::tr1::shared_ptr<TopLevelSpaceConnection> temp(where->second.lock());
            if (!temp) {
                mSpaceConnections.erase(where);
            }else if(&*temp==example) {
                mSpaceConnections.erase(where);
            }
        }
        {
            AddressConnectionMap::iterator where=mAddressConnections.find(addy);
            if (where!=mAddressConnections.end()) {
                assert(!(addy==Network::Address::null()));
                std::tr1::shared_ptr<TopLevelSpaceConnection> temp(where->second.lock());
                if (!temp) {
                    mAddressConnections.erase(where);
                }else if(&*temp==example) {
                    mAddressConnections.erase(where);
                }
            }
        }
    }
}

ProxyManager *ObjectHost::getProxyManager(const SpaceID&space) const {
    SpaceConnectionMap::const_iterator iter = mSpaceConnections.find(space);
    if (iter != mSpaceConnections.end()) {
        std::tr1::shared_ptr<TopLevelSpaceConnection> spaceConnPtr = iter->second.lock();
        if (spaceConnPtr) {
            return spaceConnPtr.get();
        }
    }
    return NULL;
}



} // namespace Sirikata


/*  Sirikata liboh -- Object Host
 *  SpaceConnection.hpp
 *
 *  Copyright (c) 2009, Patrick Reiter Horn
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
#ifndef _SIRIKATA_SPACE_CONNECTION_HPP_
#define _SIRIKATA_SPACE_CONNECTION_HPP_


#include <oh/Platform.hpp>

namespace Sirikata {
class ObjectHost;
class TopLevelSpaceConnection;


class SIRIKATA_OH_EXPORT SpaceConnection {
    std::tr1::shared_ptr<TopLevelSpaceConnection> mTopLevelStream;
    mutable Network::Stream *mStream;
  public:
    SpaceConnection(const std::tr1::shared_ptr<TopLevelSpaceConnection>&topLevel,Network::Stream*stream):
        mTopLevelStream(topLevel),mStream(stream){}
    Network::Stream * operator->()const{return mStream;}
    Network::Stream * operator*()const{return mStream;}
    class SIRIKATA_OH_EXPORT Hasher {
        size_t operator() (const SpaceConnection&sc) const;
    };
};
/*
class HostedObjectListener {
public:
    virtual void created(HostedObject *hostedObject) {
    }

    virtual void destroyed(HostedObject *hostedObject) = 0;
};

class HostedObject : public Provider<HostedObjectListener*>, ProxyObjectListener {
    SSTStream *mObjectStream;
    SpaceConnection *mSpace;
    ProxyObjectPtr mProxy;

    virtual void destroyed() {
        this->call(&HostedObjectListener::destroyed, this);
        delete this;
    }

protected:
    void creationCallback(const ProxyObjectPtr &proxyPtr) {
        mProxy = proxyPtr;
        this->call(&HostedObjectListener::created, this);
    }
public:
    // Sends initial packet on this stream, notifies ProxyManager when ready.
    HostedObject(SpaceConnection *mSpaceConnection);
    void create();
    const ProxyObjectPtr &getProxyPtr() const {
        return mProxy;
    }
    ProxyObject &getProxy() const {
        return *mProxy;
    }
};
*/
}

#endif

/*  Sirikata liboh -- Object Host
 *  HostedObject.hpp
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
#ifndef _SIRIKATA_HOSTED_OBJECT_HPP_
#define _SIRIKATA_HOSTED_OBJECT_HPP_



namespace Sirikata {
class ObjectHost;
class ProxyObject;
class TopLevelSpaceConnection;
class HostedObject : SelfWeakPtr<HostedObject> {
    class PerSpaceData {
    public:
        ObjectReference mReference;
        SpaceConnection mSpaceConnection;
        std::tr1::shared_ptr<ProxyObject> mProxyObject;
        PerSpaceData(const std::tr1::shared_ptr<TopLevelSpaceConnection>&topLevel,Network::Stream*stream);
    };
    typedef std::map<SpaceID, PerSpaceData> ObjectStreamMap;
    ObjectHost *mObjectHost;
    ObjectStreamMap mObjectStreams;
    UUID mInternalObjectReference;
    friend class ::Sirikata::SelfWeakPtr<HostedObject>;
    HostedObject(ObjectHost*parent);    
    void cloneTopLevelStream(const SpaceID&,const std::tr1::shared_ptr<TopLevelSpaceConnection>&);
    static void processMessage(const std::tr1::weak_ptr<HostedObject>&thus,const SpaceID&sid,const Network::Chunk&);
public:
    static void disconnectionEvent(const std::tr1::weak_ptr<HostedObject>&thus,const SpaceID&sid,const String&reason);
    ///makes a new objects with objectName startingLocation mesh and a space to connect to
    //FIXME implement void initialize(const UUID &objectName, const Transform&startingLocation,const String&mesh, const BoundingSphere3f&meshBounds, const SpaceID&, const std::tr1::weak_ptr<HostedObject>&spaceConnectionHint=std::tr1::weak_ptr<HostedObject>());
    ///makes a new objects with objectName startingLocation mesh and connect to some interesting space
    void initialize(const UUID &objectName, const String&script, const SpaceID&id, const std::tr1::weak_ptr<HostedObject>&spaceConnectionHint=std::tr1::weak_ptr<HostedObject>() );
    //.attempt to restore this item from database including script
    void initialize(const UUID &objectName);
    ObjectHost *getObjectHost(){return mObjectHost;}
    const ObjectHost *getObjectHost()const {return mObjectHost;}
    //FIXME implement SpaceConnection& connect(const SpaceID&space);
    //FIXME implement SpaceConnection& connect(const SpaceID&space, const SpaceConnection&example);
    bool send(RoutableMessageHeader hdr, const MemoryReference &body);
};

}

#endif

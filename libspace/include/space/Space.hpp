/*  Sirikata libspace -- Space
 *  Space.hpp
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

#ifndef _SIRIKATA_SPACE_HPP_
#define _SIRIKATA_SPACE_HPP_

#include <space/Platform.hpp>
#include <util/SpaceObjectReference.hpp>
namespace Sirikata {
class Loc;
class Oseg;
class Cseg;
class MessageRouter;
class ObjectConnections;
namespace Proximity{
class ProximitySystem;
}
class SIRIKATA_SPACE_EXPORT Space :public MessageService{
    SpaceID mID;
    Network::IOService*mIO;
    ///The registration service that allows objects to connect to the space and maps them to consistent ObjectReferences
    MessageService *mRegistration;
    ///The location services system: arbiter of object locations    
    MessageService * mLoc;
    ///The Proximity System which answers object proximity queries
    MessageService *mGeom;
    ///The object segmentation service: which Space Server hosts a given object
    Oseg *mObjectSegmentation;
    ///The coordinate segmentation service: which Space server hosts a given set of coordinates
    Cseg *mCoordinateSegmentation;
    ///The routing system to forward messages to other SpaceServers(given by mObjectSegmentation/mCoordinateSegmentation)
    MessageService *mRouter;
    ///Active connections to object hosts, with streams to individual objects;
    ObjectConnections* mObjectConnections;
    std::tr1::unordered_map<ObjectReference,MessageService*,ObjectReference::Hasher> mServices;
public:
    bool forwardMessagesTo(MessageService*){return false;}
    bool endForwardingMessagesTo(MessageService*){return false;}
    void processMessage(const ObjectReference*ref,MemoryReference message);
    void processMessage(const RoutableMessageHeader&header,
                        MemoryReference message_body);

    Space(const SpaceID&);
    ~Space();
    void run();

}; // class Space

} // namespace Sirikata

#endif //_SIRIKATA_SPACE_HPP

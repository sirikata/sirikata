/*  Sirikata
 *  ObjectHost.hpp
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

#ifndef _SIRIKATA_OBJECT_HOST_HPP_
#define _SIRIKATA_OBJECT_HOST_HPP_

#include <sirikata/oh/ObjectHostContext.hpp>
#include "ObjectHostListener.hpp"
#include <sirikata/core/service/Service.hpp>
#include <sirikata/core/service/TimeProfiler.hpp>
#include <sirikata/core/network/ObjectMessage.hpp>
#include <sirikata/core/network/SSTImpl.hpp>
#include <sirikata/core/util/MotionVector.hpp>

#include <sirikata/core/util/SerializationCheck.hpp>
#include <sirikata/core/network/Stream.hpp>
#include <sirikata/core/util/ListenerProvider.hpp>

#include <sirikata/oh/SessionManager.hpp>

namespace Sirikata {

class Object;
class ServerIDMap;

class ObjectHost : public Sirikata::Provider<ObjectHostListener*> {
public:
    // Callback indicating that a connection to the server was made and it is available for sessions
    typedef std::tr1::function<void(const SpaceID&, const ObjectReference&, ServerID, const TimedMotionVector3f&, const TimedMotionQuaternion&, const BoundingSphere3f&, const String& mesh)> ConnectedCallback;
    // Callback indicating that a connection is being migrated to a new server.  This occurs as soon
    // as the object host starts the transition and no additional notification is given since, for all
    // intents and purposes this is the point at which the transition happens
    typedef std::tr1::function<void(const SpaceID&, const ObjectReference&, ServerID)> MigratedCallback;
    typedef std::tr1::function<void(const SpaceObjectReference&)> StreamCreatedCallback;
    typedef std::tr1::function<void(const Sirikata::Protocol::Object::ObjectMessage&)> ObjectMessageCallback;
    // Notifies the ObjectHost of object connection that was closed, including a
    // reason.
    typedef std::tr1::function<void(const SpaceObjectReference&, Disconnect::Code)> DisconnectedCallback;


    // FIXME the ServerID is used to track unique sources, we need to do this separately for object hosts
    ObjectHost(ObjectHostContext* ctx, Trace::Trace* trace, ServerIDMap* sidmap);
    ~ObjectHost();

    const ObjectHostContext* context() const;

    // NOTE: The public interface is only safe to access from the main strand.

    /** Connect the object to the space with the given starting parameters. */
    void connect(Object* obj, const SolidAngle& init_sa, ConnectedCallback connected_cb,
        MigratedCallback migrated_cb, StreamCreatedCallback stream_created_cb,
        DisconnectedCallback disconnected_cb
    );
    void connect(Object* obj,
        ConnectedCallback connected_cb, MigratedCallback migrated_cb,
        StreamCreatedCallback stream_created_cb,
        DisconnectedCallback disconnected_cb
    );
    /** Disconnect the object from the space. */
    void disconnect(Object* obj);

    bool send(const Object* src, const uint16 src_port, const UUID& dest, const uint16 dest_port, const std::string& payload);

    //bool send(const uint16 src_port, const UUID& src, const uint16 dest_port, const UUID& dest,const std::string& payload);

    /* Ping Utility Methods. */
    // Fill the basic parameters in a ping message.  This is reentrant.
    void fillPing(double distance, uint32 payload_size, Sirikata::Protocol::Object::Ping* result);
    // Given a ping message constructed with fillPing(), finish constructing and
    // send it. Must be called from main strand.
    bool sendPing(const Time& t, const UUID& src, const UUID& dest, Sirikata::Protocol::Object::Ping* pmsg);
    // Construct and send a ping.  May be expensive since it needs to be
    // performed in main strand.
    bool ping(const Time& t, const UUID& src, const UUID&dest, double distance, uint32 payload_size);

    Stream<SpaceObjectReference>::Ptr getSpaceStream(const UUID& objectID);

    ///Register to intercept all incoming messages on a given port
    bool registerService(uint64 port, const ObjectMessageCallback&cb);
    ///Unregister to intercept all incoming messages on a given port
    bool unregisterService(uint64 port);

private:
    struct ConnectingInfo;

    void handleObjectConnected(const UUID& internalID, ServerID server);
    void handleObjectMigrated(const UUID& internalID, ServerID from, ServerID to);
    void handleObjectMessage(const UUID& internalID, Sirikata::Protocol::Object::ObjectMessage* msg);
    void handleObjectDisconnected(const UUID& internalID, Disconnect::Code);

    OptionSet* mStreamOptions;


    // THREAD SAFE
    // These may be accessed safely by any thread

    ObjectHostContext* mContext;

    Sirikata::SerializationCheck mSerialization;

    // Main strand only

    SessionManager mSessionManager;

    Sirikata::AtomicValue<uint32> mPingId;
    std::tr1::unordered_map<uint64, ObjectMessageCallback > mRegisteredServices;

    // Map of internal object IDs to Object*'s.
    typedef std::tr1::unordered_map<UUID, Object*, UUID::Hasher> ObjectMap;
    ObjectMap mObjects;
}; // class ObjectHost

} // namespace Sirikata


#endif //_SIRIKATA_OBJECT_HOST_HPP_

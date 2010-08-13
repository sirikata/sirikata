/*  Sirikata
 *  MonoHostedObjectExports.cpp
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

#include <sirikata/oh/Platform.hpp>
#include <sirikata/oh/HostedObject.hpp>
#include <sirikata/oh/ObjectHost.hpp>
#include <sirikata/oh/ObjectHostContext.hpp>
#include <sirikata/oh/SpaceTimeOffsetManager.hpp>
#include <sirikata/core/network/IOServiceFactory.hpp>
#include <sirikata/core/network/IOService.hpp>
#include "MonoDefs.hpp"
#include "MonoDomain.hpp"
#include "MonoContext.hpp"
#include "MonoObject.hpp"
#include "MonoDelegate.hpp"
#include "MonoArray.hpp"
#include "MonoException.hpp"
#include "MonoConvert.hpp"
#include <sirikata/core/util/SentMessage.hpp>


using namespace Sirikata;
using namespace Mono;

// Internal unique identifier of object
static void Mono_HostedObject_iInternalID(Mono::CSharpUUID* mono_result) {
    UUID uuid = MonoContext::getSingleton().getUUID();
    Mono::ConvertUUID(uuid, mono_result);
}

// ObjectReference in a Space
static void Mono_HostedObject_iObjectReference(Mono::CSharpUUID* mono_space, Mono::CSharpUUID* mono_result) {
    SpaceID space( Mono::UUIDFromMono(mono_space) );
    HostedObjectPtr vwobj = MonoContext::getSingleton().getVWObject();
    if (!vwobj) {
        Mono::ConvertUUID(UUID::null(), mono_result);
        return;
    }

    ProxyObjectPtr proxy = vwobj->getProxy(space);
    if (!proxy) {
        Mono::ConvertUUID(UUID::null(), mono_result);
        return;
    }

    ObjectReference objref = proxy->getObjectReference().object();
    Mono::ConvertUUID(objref.getAsUUID(), mono_result);
}



static void Mono_HostedObject_iLocalTime(Mono::CSharpTime* mono_result) {
    Time cur = Time::now(Sirikata::Duration::zero());
    Mono::ConvertTime(cur, mono_result);
}

static void Mono_HostedObject_iTime(Mono::CSharpUUID* mono_space, Mono::CSharpTime* mono_result) {
    SpaceID space( Mono::UUIDFromMono(mono_space) );
    Time cur = SpaceTimeOffsetManager::getSingleton().now(space);
    Mono::ConvertTime(cur, mono_result);
}



class ContextPush {
    MonoContextData mMonoContextData;
public:
    ContextPush(const Mono::Domain& domain, const HostedObjectPtr& ho) {
        MonoContext::getSingleton().push(MonoContextData(domain, ho));
    }
    ~ContextPush() {
        MonoContext::getSingleton().pop();
    }
};

static void HandleAsyncWaitTimeout(const HostedObjectWPtr& weak_ho, const Mono::Domain& domain, const Mono::Delegate& delegate) {
    HostedObjectPtr ho(weak_ho.lock());
    if (!ho) return;

    ContextPush cp(domain, ho);
    try {
        delegate.invoke();
    } catch (Mono::Exception& e) {
        SILOG(mono, debug, "AsyncWait Callback raised exception: " << e);
    }
}

static void Mono_HostedObject_iAsyncWait(Mono::CSharpDuration* mono_duration, MonoObject* callback) {
    Duration dur( Mono::DurationFromMono(mono_duration) );

    HostedObjectPtr ho = MonoContext::getSingleton().getVWObject();
    HostedObjectWPtr weak_ho = ho;
    ho->context()->ioService->post(
        dur,
        std::tr1::bind(
            &HandleAsyncWaitTimeout,
            weak_ho,
            MonoContext::getSingleton().getDomain(),
            Mono::Delegate(Mono::Object(callback)
            )
        )
    );
}



static MonoObject* Mono_HostedObject_iSendMessage(Mono::CSharpSpaceID* mono_dest_space, Mono::CSharpObjectReference* mono_dest_obj, uint32 mono_dest_port, MonoObject* mono_payload){
    std::tr1::shared_ptr<HostedObject> ho = MonoContext::getSingleton().getVWObject();
    MemoryBuffer payload;

    SpaceID dest_space( Mono::SpaceIDFromMono(mono_dest_space) );
    ObjectReference dest_obj( Mono::ObjectReferenceFromMono(mono_dest_obj) );
    ODP::PortID dest_port(mono_dest_port);
    ODP::Endpoint dest_ep(dest_space, dest_obj, dest_port);

    Mono::Array(mono_payload).unboxInPlaceByteArray(payload);

    if (!ho || payload.empty())
        return MonoContext::getSingleton().getDomain().Boolean(false).object();

    // FIXME exposing ODP ports would be a better solution
    ODP::Port* temp_port = ho->bindODPPort(dest_space);
    if (temp_port == NULL)
        return MonoContext::getSingleton().getDomain().Boolean(false).object();

    bool sent_success = temp_port->send(dest_ep, MemoryReference(payload));
    delete temp_port;

    return MonoContext::getSingleton().getDomain().Boolean(sent_success).object();
}


namespace Sirikata {

void InitHostedObjectExports () {
    // ID Functions
    mono_add_internal_call ("Sirikata.Runtime.HostedObject::iInternalID", (void*)Mono_HostedObject_iInternalID);
    mono_add_internal_call ("Sirikata.Runtime.HostedObject::iObjectReference", (void*)Mono_HostedObject_iObjectReference);
    // Time Functions
    mono_add_internal_call ("Sirikata.Runtime.HostedObject::iTime", (void*)Mono_HostedObject_iTime);
    mono_add_internal_call ("Sirikata.Runtime.HostedObject::iLocalTime", (void*)Mono_HostedObject_iLocalTime);
    // Timer Functions
    mono_add_internal_call ("Sirikata.Runtime.HostedObject::iAsyncWait", (void*)Mono_HostedObject_iAsyncWait);
    // Messaging Functions
    mono_add_internal_call ("Sirikata.Runtime.HostedObject::iSendMessage", (void*)Mono_HostedObject_iSendMessage);
}

}

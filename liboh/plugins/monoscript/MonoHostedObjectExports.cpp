#include "oh/Platform.hpp"
#include "oh/SpaceConnection.hpp"
#include "oh/HostedObject.hpp"
#include "oh/ObjectHost.hpp"
#include "oh/SpaceTimeOffsetManager.hpp"
#include "network/IOServiceFactory.hpp"
#include "network/IOService.hpp"
#include "MonoDefs.hpp"
#include "MonoDomain.hpp"
#include "MonoContext.hpp"
#include "MonoObject.hpp"
#include "MonoDelegate.hpp"
#include "MonoArray.hpp"
#include "MonoException.hpp"
#include "MonoConvert.hpp"
#include "util/SentMessage.hpp"


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

static void CallDelegate(const std::tr1::weak_ptr<HostedObject>&weak_ho, const Mono::Domain &domain, const Mono::Delegate&delegate) {
    std::tr1::shared_ptr<HostedObject> ho(weak_ho.lock());
    if (ho) {
        ContextPush cp(domain, ho);
        try {
            delegate.invoke();
        } catch (Mono::Exception&e) {
            SILOG(mono,debug,"AsyncWait Callback raised exception: "<<e);
        }
    }
}
static void Mono_Context_AsyncWait(MonoObject*callback, MonoObject*duration){
    std::tr1::weak_ptr<HostedObject> ho=MonoContext::getSingleton().getVWObject();
    MonoContext::getSingleton().getVWObject()->getObjectHost()->getSpaceIO()->post(
                                             Mono::Object(duration).unboxDuration(),
                                             std::tr1::bind(&CallDelegate,ho,MonoContext::getSingleton().getDomain(), Mono::Delegate(Mono::Object(callback))));
}



static MonoObject* Mono_HostedObject_SendMessage(Mono::CSharpSpaceID* mono_dest_space, Mono::CSharpObjectReference* mono_dest_obj, uint32 mono_dest_port, MonoObject* mono_payload){
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


static void Mono_Context_TickDelay(MonoObject*duration) {
    std::tr1::shared_ptr<HostedObject> ho=MonoContext::getSingleton().getVWObject();
    if (ho) {
        Sirikata::Duration period(Mono::Object(duration).unboxTime()-Time::epoch());
        NOT_IMPLEMENTED(mono);
        //ho->tickDelay(period)
    }
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
    mono_add_internal_call ("Sirikata.Runtime.HostedObject::iAsyncWait", (void*)Mono_Context_AsyncWait);
    mono_add_internal_call ("Sirikata.Runtime.HostedObject::iTickPeriod", (void*)Mono_Context_TickDelay);
    // Messaging Functions
    mono_add_internal_call ("Sirikata.Runtime.HostedObject::iSendMessage", (void*)Mono_HostedObject_SendMessage);
}

}

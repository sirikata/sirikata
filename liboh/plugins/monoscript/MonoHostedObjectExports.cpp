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


static void Mono_Context_CallFunctionCallback(const std::tr1::weak_ptr<HostedObject>&weak_ho,
                                              const Mono::Domain &domain,
                                              const Mono::Delegate &callback,
                                              SentMessage*sentMessage,
                                              const RoutableMessageHeader&responseHeader,
                                              MemoryReference responseBody) {
    std::tr1::shared_ptr<HostedObject>ho(weak_ho);
    bool keepquery = false;
    if (ho) {
        MonoContext::getSingleton().push(MonoContextData(domain, ho));
        String header;
        responseHeader.SerializeToString(&header);
        try {
            Object ret = callback.invoke(MonoContext::getSingleton().getDomain().ByteArray(header.data(),
                                                                              header.size()),
                            MonoContext::getSingleton().getDomain().ByteArray(responseBody.data(),
                                                                              responseBody.size()));
            if (!ret.null()) {
                keepquery = ret.unboxBoolean();
            }
        }catch (Mono::Exception&e) {
            SILOG(mono,debug,"Callback raised exception: "<<e);
            keepquery = false;
        }
        MonoContext::getSingleton().pop();
    }
    if (!keepquery) {
        delete sentMessage;
    }
}
static MonoObject* InternalMono_Context_CallFunction(MonoObject *message, MonoObject*callback, const Sirikata::Duration&duration){
    std::tr1::shared_ptr<HostedObject> ho=MonoContext::getSingleton().getVWObject();
    MemoryBuffer buf;
    using std::tr1::placeholders::_1;
    using std::tr1::placeholders::_2;
    using std::tr1::placeholders::_3;
    Mono::Array(message).unboxInPlaceByteArray(buf);
    if (ho&&!buf.empty()) {
        RoutableMessageHeader hdr;
        MemoryReference body=hdr.ParseFromArray(&buf[0],buf.size());
        SpaceID dest_space = hdr.destination_space();
        SentMessage*sm=hdr.has_id()
            ? new SentMessage(hdr.id(),ho->getTracker(dest_space),std::tr1::bind(&Mono_Context_CallFunctionCallback,
                                                                       ho->getWeakPtr(),
                                                                       MonoContext::getSingleton().getDomain(),
                                                                       Mono::Delegate(Mono::Object(callback)),
                                       _1,_2,_3))
            :new SentMessage(ho->getTracker(dest_space),std::tr1::bind(&Mono_Context_CallFunctionCallback,
                                                             ho->getWeakPtr(),
                                                             MonoContext::getSingleton().getDomain(),
                                                             Mono::Delegate(Mono::Object(callback)),
                                                             _1,_2,_3));

        sm->setTimeout(duration);
        sm->header()=hdr;
        sm->send(body);
    }else {
        return MonoContext::getSingleton().getDomain().Boolean(false).object();
    }
    return MonoContext::getSingleton().getDomain().Boolean(true).object();
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



static MonoObject* Mono_Context_CallFunctionWithTimeout(MonoObject *message, MonoObject*callback, MonoObject*duration){
    return InternalMono_Context_CallFunction(message,callback,Mono::Object(duration).unboxDuration());
}
static MonoObject* Mono_Context_CallFunction(MonoObject *message, MonoObject*callback){
    return InternalMono_Context_CallFunction(message,callback,Sirikata::Duration::seconds(4.0));
}
static MonoObject* Mono_Context_SendMessage(Mono::CSharpSpaceID* mono_dest_space, Mono::CSharpObjectReference* mono_dest_obj, uint32 mono_dest_port, MonoObject* mono_payload){
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

void
InitHostedObjectExports () {
    mono_add_internal_call ("Sirikata.Runtime.HostedObject::iInternalID", (void*)Mono_HostedObject_iInternalID);
    mono_add_internal_call ("Sirikata.Runtime.HostedObject::iObjectReference", (void*)Mono_HostedObject_iObjectReference);

    mono_add_internal_call ("Sirikata.Runtime.HostedObject::iTime", (void*)Mono_HostedObject_iTime);
    mono_add_internal_call ("Sirikata.Runtime.HostedObject::iLocalTime", (void*)Mono_HostedObject_iLocalTime);


    mono_add_internal_call ("Sirikata.Runtime.HostedObject::iSendMessage", (void*)Mono_Context_SendMessage);
    mono_add_internal_call ("Sirikata.Runtime.HostedObject::iCallFunction", (void*)Mono_Context_CallFunction);
    mono_add_internal_call ("Sirikata.Runtime.HostedObject::iCallFunctionWithTimeout", (void*)Mono_Context_CallFunction);
    mono_add_internal_call ("Sirikata.Runtime.HostedObject::iAsyncWait", (void*)Mono_Context_AsyncWait);
    mono_add_internal_call ("Sirikata.Runtime.HostedObject::iTickPeriod", (void*)Mono_Context_TickDelay);
}

}

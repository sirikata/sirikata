#include "oh/Platform.hpp"
#include "oh/SpaceConnection.hpp"
#include "oh/HostedObject.hpp"
#include "MonoDefs.hpp"
#include "MonoDomain.hpp"
#include "MonoContext.hpp"
#include "MonoObject.hpp"
#include "MonoDelegate.hpp"
#include "MonoArray.hpp"
#include "MonoException.hpp"
#include "util/SentMessage.hpp"
using namespace Sirikata;
using namespace Mono;
static MonoObject* Mono_Context_CurrentUUID() {
    UUID uuid = MonoContext::getSingleton().getUUID();
    Mono::Object obj = MonoContext::getSingleton().getDomain().UUID(uuid);
    return obj.object();
}
/*
static MonoObject* Mono_Context_CurrentObject() {
    Mono::Object obj =  Mono::Domain::root().UUID(MonoContext::getSingleton().getVWObject());
    return obj.object();
}

static MonoObject* Mono_Context_CallFunction(MonoObject*callback) {
    Mono::Object obj =  Mono::Domain::root().UUID(MonoContext::getSingleton().getVWObject());
    return obj.object();
}
*/
static void Mono_Context_CallFunctionCallback(const std::tr1::weak_ptr<HostedObject>&weak_ho,
                                              const Mono::Domain &domain,
                                              const Mono::Delegate &callback,
                                              SentMessage*sentMessage,
                                              const RoutableMessageHeader&responseHeader,
                                              MemoryReference responseBody) {
    std::tr1::shared_ptr<HostedObject>ho(weak_ho);
    if (ho) {
        MonoContext::getSingleton().push(MonoContextData());
        MonoContext::getSingleton().setVWObject(&*ho,domain);
        String header;
        responseHeader.SerializeToString(&header);
        try {
            callback.invoke(MonoContext::getSingleton().getDomain().ByteArray(header.data(),
                                                                              header.size()),
                            MonoContext::getSingleton().getDomain().ByteArray(responseBody.data(),
                                                                              responseBody.size()));
        }catch (Mono::Exception&e) {
            SILOG(mono,debug,"Callback raised exception: "<<e);
        }
        MonoContext::getSingleton().pop();
    }
    delete sentMessage;
}
static MonoObject* InternalMono_Context_CallFunction(MonoObject *message, MonoObject*callback, const Duration&duration){
    std::tr1::shared_ptr<HostedObject> ho=MonoContext::getSingleton().getVWObject();
    MemoryBuffer buf;
    
    Mono::Array(message).unboxInPlaceByteArray(buf);
    if (ho&&!buf.empty()) {
        RoutableMessageHeader hdr;
        MemoryReference body=hdr.ParseFromArray(&buf[0],buf.size());
        SentMessage*sm=hdr.has_id()?new SentMessage(hdr.id(),ho->getTracker()):new SentMessage(ho->getTracker());
        sm->setCallback(std::tr1::bind(&Mono_Context_CallFunctionCallback,
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
static MonoObject* Mono_Context_CallFunctionWithTimeout(MonoObject *message, MonoObject*callback, MonoObject*duration){
    return InternalMono_Context_CallFunction(message,callback,Mono::Object(duration).unboxTime()-Time::epoch());
}
static MonoObject* Mono_Context_CallFunction(MonoObject *message, MonoObject*callback){
    return InternalMono_Context_CallFunction(message,callback,Duration::seconds(4.0));
}
static MonoObject* Mono_Context_SendMessage(MonoObject *message){
    std::tr1::shared_ptr<HostedObject> ho=MonoContext::getSingleton().getVWObject();
    MemoryBuffer buf;
    
    Mono::Array(message).unboxInPlaceByteArray(buf);
    if (ho&&!buf.empty()) {
        RoutableMessageHeader hdr;
        MemoryReference body=hdr.ParseFromArray(&buf[0],buf.size());
        
        ho->send(hdr,body);
    }else {
        return MonoContext::getSingleton().getDomain().Boolean(false).object();
    }
    return MonoContext::getSingleton().getDomain().Boolean(true).object();
}

static void Mono_Context_TickDelay(MonoObject*duration) {
    std::tr1::shared_ptr<HostedObject> ho=MonoContext::getSingleton().getVWObject();
    if (ho) {
        Duration period(Mono::Object(duration).unboxTime()-Time::epoch());
        NOT_IMPLEMENTED(mono);
        //ho->tickDelay(period)
    }
}

namespace Sirikata {

void 
InitHostedObjectExports () {
    mono_add_internal_call ("Sirikata.Runtime.HostedObject::GetUUID", (void*)Mono_Context_CurrentUUID);
    mono_add_internal_call ("Sirikata.Runtime.HostedObject::iSendMessage", (void*)Mono_Context_SendMessage);
    mono_add_internal_call ("Sirikata.Runtime.HostedObject::iCallFunction", (void*)Mono_Context_CallFunction);
    mono_add_internal_call ("Sirikata.Runtime.HostedObject::iCallFunctionWithTimeout", (void*)Mono_Context_CallFunction);
    mono_add_internal_call ("Sirikata.Runtime.HostedObject::iTickPeriod", (void*)Mono_Context_TickDelay);
}
}

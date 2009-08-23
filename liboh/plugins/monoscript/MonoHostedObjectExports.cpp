#include "oh/Platform.hpp"
#include "oh/SpaceConnection.hpp"
#include "oh/HostedObject.hpp"
#include "oh/SpaceTimeOffsetManager.hpp"
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
    bool keepquery = false;
    if (ho) {
        MonoContext::getSingleton().push(MonoContextData());
        MonoContext::getSingleton().setVWObject(&*ho,domain);
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
static MonoObject* InternalMono_Context_CallFunction(MonoObject *message, MonoObject*callback, const Duration&duration){
    std::tr1::shared_ptr<HostedObject> ho=MonoContext::getSingleton().getVWObject();
    MemoryBuffer buf;
    
    Mono::Array(message).unboxInPlaceByteArray(buf);
    if (ho&&!buf.empty()) {
        RoutableMessageHeader hdr;
        MemoryReference body=hdr.ParseFromArray(&buf[0],buf.size());
        SentMessage*sm=hdr.has_id()
            ? new SentMessage(hdr.id(),ho->getTracker(),std::tr1::bind(&Mono_Context_CallFunctionCallback,
                                                                       ho->getWeakPtr(),
                                                                       MonoContext::getSingleton().getDomain(),
                                                                       Mono::Delegate(Mono::Object(callback)),
                                       _1,_2,_3))
            :new SentMessage(ho->getTracker(),std::tr1::bind(&Mono_Context_CallFunctionCallback,
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

void Mono_Context_setTime(MonoObject *timeRetval, const Time& cur) {
    uint64 curRaw=cur.raw();
    Mono::Object retval(timeRetval);
    uint32 curLowerLower=curRaw%65536;
    uint32 curLowerUpper=(curRaw/65536)%65536;
    uint32 curLower=curLowerLower+curLowerUpper*65536;
    curRaw/=65536;
    curRaw/=65536;
    uint32 curUpper=curRaw;
    retval.send("setLowerUpper",
                MonoContext::getSingleton().getDomain().UInt32(curLower),
                MonoContext::getSingleton().getDomain().UInt32(curUpper)); 
}
static void Mono_Context_GetTime(MonoObject*space_id,MonoObject *timeRetval) {
    SpaceID sid=SpaceID(Mono::Object(space_id).unboxUUID());
    Time cur=SpaceTimeOffsetManager::getSingleton().now(sid);
    Mono_Context_setTime(timeRetval,cur);
}


static void Mono_Context_GetLocalTime(MonoObject *timeRetval) {
    Time cur=Time::now(Duration::zero());
    //SILOG(monoscript,warning,"Time should be "<<cur.raw());
    Mono_Context_setTime(timeRetval,cur);
}
static void Mono_Context_GetTimeByteArray(MonoObject*space_id,MonoObject *timeRetval) {
    MemoryBuffer buf;
    Mono::Array(space_id).unboxInPlaceByteArray(buf);
    if (buf.size()==16) {
        SpaceID sid=SpaceID(*Sirikata::Array<unsigned char, 16,true>().memcpy(&buf[0],buf.size()));
        Time cur= SpaceTimeOffsetManager::getSingleton().now(sid);
        //SILOG(monoscript,warning,"Time should be "<<cur.raw());
        Mono_Context_setTime(timeRetval,cur);
    }else {
         Mono_Context_GetLocalTime(timeRetval);
    }
}

static void Mono_Context_GetTimeString(MonoObject*space_id, MonoObject* timeRetval) {
    std::string ss=Mono::Object(space_id).unboxString();
    try {
        SpaceID sid=SpaceID(UUID(ss,UUID::HumanReadable()));
        Time cur = SpaceTimeOffsetManager::getSingleton().now(sid);
        Mono_Context_setTime(timeRetval,cur);
    }catch (std::invalid_argument&ia){
         Mono_Context_GetLocalTime(timeRetval);
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
    mono_add_internal_call ("Sirikata.Runtime.HostedObject::iGetTime", (void*)Mono_Context_GetTime);
    mono_add_internal_call ("Sirikata.Runtime.HostedObject::iGetTimeByteArray", (void*)Mono_Context_GetTimeByteArray);
    mono_add_internal_call ("Sirikata.Runtime.HostedObject::iGetTimeString", (void*)Mono_Context_GetTimeString);
    mono_add_internal_call ("Sirikata.Runtime.HostedObject::iGetLocalTime", (void*)Mono_Context_GetLocalTime);
}
}

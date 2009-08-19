/*  Sirikata liboh -- Object Host
 *  MonoObjectScript.cpp
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
#include "oh/Platform.hpp"
#include "MonoVWObjectScriptManager.hpp"
#include "MonoVWObjectScript.hpp"
#include "MonoSystem.hpp"
#include "MonoClass.hpp"
#include "MonoAssembly.hpp"
#include "MonoArray.hpp"
#include "MonoException.hpp"
#include "util/RoutableMessageHeader.hpp"
#include "MonoContext.hpp"
namespace Sirikata {

MonoVWObjectScript::MonoVWObjectScript(Mono::MonoSystem*mono_system, HostedObject*ho, const ObjectScriptManager::Arguments&args):mDomain(mono_system->createDomain()){
    mParent=ho;
    int ignored_args=0;
    String reserved_string_assembly="Assembly";
    String reserved_string_class="Class";
    String reserved_string_namespace="Namespace";
    String reserved_string_function="Function";
    String assembly_name;//="Sirikata.Runtime";
    ObjectScriptManager::Arguments::const_iterator i=args.begin(),j,func_iter;
    if ((i=args.find(reserved_string_assembly))!=args.end()) {
        assembly_name=i->second;
        ++ignored_args;
    }
    mono_system->loadAssembly(assembly_name);
    try {
        Mono::Assembly ass=mDomain.getAssembly(i->second);
        String class_name;//="PythonObject";
        String namespace_name;//="Sirikata.Runtime";
        
        if ((i=args.find(reserved_string_class))!=args.end()) {        
            ++ignored_args;
            class_name=i->second;
        }
        if ((i=args.find(reserved_string_namespace))!=args.end()) {        
            ++ignored_args;
            namespace_name=i->second;
        }
        if ((func_iter=args.find(reserved_string_function))!=args.end()) {        
            ++ignored_args;
        }
        MonoContext::getSingleton().push(MonoContextData());
        MonoContext::getSingleton().setVWObject(ho,mDomain);
        try {
            
            Mono::Class class_type=ass.getClass(namespace_name,class_name);
            Mono::Object exampleString=mDomain.String(String());
            Mono::Array mono_args=Mono::Array(mDomain.Array(exampleString.type(),(args.size()-ignored_args)*2));
            unsigned int mono_count=0;
            
            for (i=args.begin(),j=args.end();i!=j;++i) {
                if (i->first!=reserved_string_assembly&&i->first!=reserved_string_class&&i->first!=reserved_string_namespace&&i->first!=reserved_string_function) {                        
                    mono_args.set(mono_count++,mDomain.String(i->first));
                    mono_args.set(mono_count++,mDomain.String(i->second));
                }
            }
            try {
                if (func_iter==args.end()) {
                    mObject=class_type.instance(mono_args);
                }else {
                    mObject=class_type.send(func_iter->second,mono_args);
                }
            }catch(Mono::Exception&e) {
                SILOG(mono,warning,"Making new object: Cannot locate method "<<(func_iter==args.end()?String("constructor"):func_iter->second) <<" in class "<<namespace_name<<"::"<<class_name<<"with "<<mono_args.length()<<" arguments."<<e);
            }
        } catch (Mono::Exception&e) {
            SILOG(mono,warning,"Making new object: Cannot locate class "<<namespace_name<<"::"<<class_name<<"."<<e);
        }
        MonoContext::getSingleton().pop();
    } catch (Mono::Exception&e) {
        SILOG(mono,warning,"Making new object: Cannot locate assembly "<<i->second<<"."<<e);
        //no assembly could be loaded
    }
}
MonoVWObjectScript::~MonoVWObjectScript(){

    //mono_jit_cleanup(mDomain.domain());
}
bool MonoVWObjectScript::forwardMessagesTo(MessageService*){
    NOT_IMPLEMENTED(mono);
    return false;
}
bool MonoVWObjectScript::endForwardingMessagesTo(MessageService*){
    NOT_IMPLEMENTED(mono);
    return false;
}
bool MonoVWObjectScript::processRPC(const RoutableMessageHeader &receivedHeader, const std::string &name, MemoryReference args, MemoryBuffer &returnValue){
    MonoContext::getSingleton().push(MonoContextData());
    MonoContext::getSingleton().setVWObject(mParent,mDomain);
    std::string header;
    receivedHeader.SerializeToString(&header);
    try {
        Mono::Object retval=mObject.send("processRPC",mDomain.ByteArray(header.data(),(unsigned int)header.size()),mDomain.String(name),mDomain.ByteArray((const char*)args.data(),(int)args.size()));
        if (!retval.null()) {
            returnValue=retval.unboxByteArray();
            MonoContext::getSingleton().pop();
            return true;
        }
        MonoContext::getSingleton().pop();
        return false;
    }catch (Mono::Exception&e) {
        SILOG(mono,debug,"RPC Exception "<<e);
        MonoContext::getSingleton().pop();
        return false;        
    }
    MonoContext::getSingleton().pop();
    return true;
}
void MonoVWObjectScript::tick(){
    MonoContext::getSingleton().push(MonoContextData());
    MonoContext::getSingleton().setVWObject(mParent,mDomain);
    try {
        Mono::Object retval=mObject.send("tick",mDomain.Time(Time::now(Duration::zero())));
    }catch (Mono::Exception&e) {
        SILOG(mono,debug,"Tick Exception "<<e);
    }
    MonoContext::getSingleton().pop();
}
void MonoVWObjectScript::processMessage(const RoutableMessageHeader&receivedHeader , MemoryReference body){
    std::string header;
    receivedHeader.SerializeToString(&header);
    MonoContext::getSingleton().push(MonoContextData());
    MonoContext::getSingleton().setVWObject(mParent,mDomain);
    try {
        Mono::Object retval=mObject.send("processMessage",mDomain.ByteArray(header.data(),(unsigned int)header.size()),mDomain.ByteArray((const char*)body.data(),(unsigned int)body.size()));
    }catch (Mono::Exception&e) {
        SILOG(mono,debug,"Message Exception "<<e);
    }
    MonoContext::getSingleton().pop();
}


}

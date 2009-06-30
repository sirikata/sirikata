/*  Sirikata libspace -- Location Services
 *  Loc.cpp
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

#include <space/Loc.hpp>
#include <space/Registration.hpp>
#include "Space_Sirikata.pbj.hpp"
#include "util/RoutableMessage.hpp"
#include "util/KnownServices.hpp"
namespace Sirikata {
Loc::Loc(){
    
}

Loc::~Loc() {

}

bool Loc::forwardMessagesTo(MessageService*ms) {
    mServices.push_back(ms);
    return true;
}

bool Loc::endForwardingMessagesTo(MessageService*ms) {
    std::vector<MessageService*>::iterator where=std::find(mServices.begin(),mServices.end(),ms);
    if (where==mServices.end())
        return false;
    mServices.erase(where);
    return true;
}

void Loc::processMessage(const ObjectReference&object_reference,const Protocol::ObjLoc&loc){
    RoutableMessageBody body;
    body.add_message_arguments(std::string());
    loc.SerializeToString(&body.message_arguments(0));
    std::string message_body;
    body.SerializeToString(&message_body);
    RoutableMessageHeader destination_header;
    destination_header.set_source_object(ObjectReference::spaceServiceID());
    destination_header.set_source_port(Services::LOC);
    destination_header.set_destination_object(object_reference);
    
    for (std::vector<MessageService*>::iterator i=mServices.begin(),ie=mServices.end();i!=ie;++i) {
        (*i)->processMessage(destination_header,MemoryReference(message_body));
    }
   
}
void Loc::processMessage(const RoutableMessageHeader&header,MemoryReference message_body) {
    RoutableMessageBody body;
    if (body.ParseFromArray(message_body.data(),message_body.size())) {
        int num_args=body.message_arguments_size();
        if (header.has_source_object()&&header.source_object()==ObjectReference::spaceServiceID()&&header.source_port()==Services::REGISTRATION) {
            for (int i=0;i<num_args;++i) {
                if (body.message_names_serialized_size()==0||body.message_names(i)=="RetObj") {
                    Protocol::RetObj retObj;
                    if (retObj.ParseFromString(body.message_arguments(i))&&retObj.has_location()) {
                        processMessage(ObjectReference(retObj.object_reference()),retObj.location());
                    }else {
                        SILOG(loc,warning,"Loc:Unable to parse RetObj message body originating from "<<header.source_object());
                    }
                }
            }
        }else {
            for (int i=0;i<num_args;++i) {
                Protocol::ObjLoc objLoc;
                if (objLoc.ParseFromString(body.message_arguments(i))) {
                    processMessage(ObjectReference(header.source_object()),objLoc);
                }else {
                    SILOG(loc,warning,"Loc:Unable to parse ObjLoc message body originating from "<<header.source_object());
                }
            }           
            
        }
    }else {
        SILOG(loc,warning,"Loc:Unable to parse message body originating from "<<header.source_object());
    }
}
}

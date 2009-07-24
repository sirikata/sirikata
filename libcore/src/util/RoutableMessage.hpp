/*  Sirikata Messages
 *  RoutableMessage.hpp
 *
 *  Copyright (c) 2008, Daniel Reiter Horn
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

#ifndef _SIRIKATA_ROUTABLE_MESSAGE_HPP_
#define _SIRIKATA_ROUTABLE_MESSAGE_HPP_

#include "RoutableMessageHeader.hpp"
#include "RoutableMessageBody.hpp"
namespace Sirikata {
class RoutableMessage:public RoutableMessageHeader, private RoutableMessageBody {
    static const int sMaxInt=2147483647;
public:
    RoutableMessage(const RoutableMessageHeader&hdr, const void*body, const size_t body_size):RoutableMessageHeader(hdr) {

        if (body_size>(size_t)sMaxInt||!this->Sirikata::RoutableMessageBody::ParseFromArray(body,body_size)) {
            throw std::invalid_argument("incomplete body message");
        }
    }
    RoutableMessage(const RoutableMessageHeader&hdr):RoutableMessageHeader(hdr) {
    }
    RoutableMessage(const RoutableMessageHeader&hdr, const RoutableMessageBody&body):RoutableMessageHeader(hdr),RoutableMessageBody(body) {
    }
    RoutableMessage(){}
    RoutableMessageBody& body(){return *this;}
    const RoutableMessageBody& body()const {return *this;}
    Sirikata::RoutableMessageHeader& header(){return *this;}
    const Sirikata::RoutableMessageHeader& header()const {return *this;}
    bool ParseFromArray(const void*input,size_t size) {
        MemoryReference body=this->RoutableMessageHeader::ParseFromArray(input,size);
        if (body.size()>(size_t)sMaxInt)
            return false;
        int isize=(int)body.size();
        return this->Sirikata::RoutableMessageBody::ParseFromArray(body.data(),isize);
    }
    bool ParsePartialFromArray(const void*input,size_t size) {
        MemoryReference body=this->RoutableMessageHeader::ParseFromArray(input,size);
        if (body.size()>(size_t)sMaxInt)
            return false;
        int isize=(int)body.size();
        return this->Sirikata::RoutableMessageBody::ParsePartialFromArray(body.data(),isize);
    }
    bool ParseFromString(const std::string&data) {
        return ParseFromArray(data.data(),data.length());
    }
    bool ParsePartialFromString(const std::string&data) {
        return ParsePartialFromArray(data.data(),data.length());
    }
    bool SerializeToString(std::string*str) const {
        return this->RoutableMessageHeader::SerializeToString(str)&&
            this->Sirikata::RoutableMessageBody::AppendToString(str);
    }
    bool AppendToString(std::string*str) const {
        return this->RoutableMessageHeader::AppendToString(str)&&
            this->Sirikata::RoutableMessageBody::AppendToString(str);
    }
};


template <class Msg> class AppendMessage {public:
    static void toString(const Msg&const_msg,std::string*str) {
        Msg *msg=const_cast<Msg*>(&const_msg);
        RoutableMessage rm;
        if (msg->has_source_object()) {
            rm.set_source_object(msg->source_object());
            msg->clear_source_object();
        }
        if (msg->has_destination_object()) {
            rm.set_destination_object(msg->destination_object());
            msg->clear_destination_object();
        }
        if (msg->has_source_space()) {
            rm.set_source_space(msg->source_space());
            msg->clear_source_space();
        }
        if (msg->has_destination_space()) {
            rm.set_destination_space(msg->destination_space());
            msg->clear_destination_space();
        }
        rm.AppendToString(str);
        msg->AppendToString(str);
        if (rm.has_source_object()) {
            msg->set_source_object(rm.source_object());
        }
        if (rm.has_destination_object()) {
            msg->set_destination_object(rm.destination_object());
        }
        if (rm.has_source_space()) {
            msg->set_source_space(rm.source_space());
        }
        if (rm.has_destination_space()) {
            msg->set_destination_space(rm.destination_space());
        }
    }
    static void SerializeToString(const Msg&const_msg,std::string*str) {
        str->resize(0);
        AppendToString(const_msg,str);
    }
};

}
#endif

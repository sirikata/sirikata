#ifndef _SIRIKATA_ROUTABLE_MESSAGE_HPP_
#define _SIRIKATA_ROUTABLE_MESSAGE_HPP_

#include "RoutableMessageHeader.hpp"
namespace Sirikata {
class RoutableMessage:public RoutableMessageHeader, private Sirikata::Protocol::MessageBody {
    static const int sMaxInt=2147483647;
public:
    RoutableMessage(const RoutableMessageHeader&hdr, const void*body, const size_t body_size):RoutableMessageHeader(hdr) {
        
        if (body_size>(size_t)sMaxInt||!this->Sirikata::Protocol::MessageBody::ParseFromArray(body,body_size)) {
            throw std::invalid_argument("incomplete body message");
        }
    }
    RoutableMessage(const RoutableMessageHeader&hdr):RoutableMessageHeader(hdr) {
    }
    RoutableMessage(const RoutableMessageHeader&hdr, const Protocol::MessageBody&body):RoutableMessageHeader(hdr),Protocol::MessageBody(body) {
    }
    RoutableMessage(){}
    Sirikata::Protocol::MessageBody& body(){return *this;}
    const Sirikata::Protocol::MessageBody& body()const {return *this;}
    Sirikata::RoutableMessageHeader& header(){return *this;}
    const Sirikata::RoutableMessageHeader& header()const {return *this;}
    bool ParseFromArray(const void*input,size_t size) {
        MemoryReference body=this->RoutableMessageHeader::ParseFromArray(input,size);
        if (body.size()>(size_t)sMaxInt)
            return false;
        int isize=(int)body.size();
        return this->Sirikata::Protocol::MessageBody::ParseFromArray(body.data(),isize);
    }
    bool ParsePartialFromArray(const void*input,size_t size) {
        MemoryReference body=this->RoutableMessageHeader::ParseFromArray(input,size);
        if (body.size()>(size_t)sMaxInt)
            return false;
        int isize=(int)body.size();
        return this->Sirikata::Protocol::MessageBody::ParsePartialFromArray(body.data(),isize);
    }
    bool ParseFromString(const std::string&data) {
        return ParseFromArray(data.data(),data.length());
    }
    bool ParsePartialFromString(const std::string&data) {
        return ParsePartialFromArray(data.data(),data.length());
    }
    bool SerializeToString(std::string*str) const {
        return this->RoutableMessageHeader::SerializeToString(str)&&
            this->Sirikata::Protocol::MessageBody::AppendToString(str);
    }
    bool AppendToString(std::string*str) const {
        return this->RoutableMessageHeader::AppendToString(str)&&
            this->Sirikata::Protocol::MessageBody::AppendToString(str);
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

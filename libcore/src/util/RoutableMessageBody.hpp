/*  Sirikata Utilities -- Message Packet Header Parser
 *  RoutableMessageBody.hpp
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
#ifndef _SIRIKATA_ROUTABLE_MESSAGE_BODY_HPP_
#define _SIRIKATA_ROUTABLE_MESSAGE_BODY_HPP_
namespace Sirikata {
class RoutableMessageBody: private Protocol::MessageBody {
    std::string& getInternalDefaultMessageName() const{
        static std::string default_string;
        return default_string;
    }
public:
    RoutableMessageBody() {
    }
    const std::string& getDefaultMessageName()const {return getInternalDefaultMessageName();}


    int message_size()const {
        int namesize=this->Protocol::MessageBody::message_names_size();
        int argsize=this->Protocol::MessageBody::message_arguments_size();
        if (argsize>namesize) return argsize;
        return namesize;
    }
    std::string& message_names(int i) {
        int namesize=this->Protocol::MessageBody::message_names_size();
        int argsize=this->Protocol::MessageBody::message_arguments_size();
        if (i >= 0 && i<namesize) {
            return this->Protocol::MessageBody::message_names(i);
        }
        if (namesize) {
            return this->Protocol::MessageBody::message_names(namesize-1);
        }
        return getInternalDefaultMessageName();
    }
    const std::string& message_names(int i) const{
        int namesize=this->Protocol::MessageBody::message_names_size();
        int argsize=this->Protocol::MessageBody::message_arguments_size();
        if (i >= 0 && i<namesize) {
            return this->Protocol::MessageBody::message_names(i);
        }
        if (namesize) {
            return this->Protocol::MessageBody::message_names(namesize-1);
        }
        return getDefaultMessageName();
    }
    std::string* add_message(const std::string &name, const std::string &arguments=std::string()) {
        return add_message(name, arguments.data(), arguments.length());
    }
    std::string* add_message(const std::string &name, const void *args, size_t len) {
        if (!message_names_size()) {
            add_message_names(name);
        }
        if (name != Protocol::MessageBody::message_names(message_names_size()-1)) {
            while (message_names_size() < message_arguments_size()) {
                add_message_names(Protocol::MessageBody::message_names(message_names_size()-1));
            }
            add_message_names(name);
        }
        add_message_arguments(args, len);
        return &Protocol::MessageBody::message_arguments(message_arguments_size()-1);
    }
    void add_message_reply(std::string args) {
        add_message_arguments(args);
    }
    void add_message_reply(const void *args, size_t len) {
        add_message_arguments(args, len);
    }
    std::string *add_message_reply() {
        add_message_arguments(std::string());
        return &message_arguments(message_arguments_size()-1);
    }
    void clear_message() {
        clear_message_names();
        clear_message_arguments();
    }

    using Protocol::MessageBody::message_arguments;
    using Protocol::MessageBody::ParseFromArray;
    using Protocol::MessageBody::ParseFromString;
    using Protocol::MessageBody::SerializeToString;
    using Protocol::MessageBody::AppendToString;
};
}
#endif

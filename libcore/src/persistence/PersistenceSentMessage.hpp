/*  Sirikata persistence -- Database Services
 *  PersistenceSentMessage.hpp
 *
 *  Copyright (c) 2009, Patrick Reiter Horn
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
#ifndef SIRIKATA_PersistenceSentMessage_hpp
#define SIRIKATA_PersistenceSentMessage_hpp

#include "util/SentMessage.hpp"

namespace Sirikata {
namespace Persistence {

template <class MessageType> class PersistenceSentMessage : public SentMessageBody<MessageType> {
protected:
    typedef PersistenceSentMessage<MessageType> ThisClass;
    typedef SentMessageBody<MessageType> SuperClass;

public:
    static ThisClass* cast_sent_message(SentMessage * parent) {
        return static_cast<ThisClass*>(parent);        
    }
    static const ThisClass* cast_sent_message(const SentMessage * parent) {
        return static_cast<const ThisClass*>(parent);
    }
    typedef std::tr1::function<void(
        ThisClass *msg,
        const RoutableMessageHeader &lastHeader,
        Protocol::Response::ReturnStatus errorCode)> PersistenceCallback;

private:
    PersistenceCallback mRealCallback;

    static void receivedPersistenceMessageStatic(
        SentMessage* sentMessage,
        const RoutableMessageHeader &responseHeader,
        MemoryReference responseBody)
    {
        static_cast<ThisClass*>(sentMessage)
            ->receivedPersistenceMessage(responseHeader, responseBody);
    }
    void receivedPersistenceMessage(
        const RoutableMessageHeader &responseHeader,
        MemoryReference responseBody)
    {
        if (responseHeader.has_return_status()) {
            mRealCallback(this, responseHeader, Protocol::Response::SUCCESS);
            return;
        }
        Protocol::Response resp;
        resp.ParseFromArray(responseBody.data(), responseBody.length());
        if (resp.has_return_status() && resp.return_status()) {
            mRealCallback(this, responseHeader, resp.return_status());
            return;
        }
        if (resp.reads_size() == 0) {
            SILOG(persistence,info,"Persistence sent message return 0");
            return;
        }
        for (int i = 0, index = 0; i < resp.reads_size(); i++, index++) {
            if (resp.reads(i).has_index()) {
                index = resp.reads(i).index();
            }
            if (index >= 0 && index < body().reads_size()) {
                if (resp.reads(i).has_data()) {
                    if (!body().reads(index).has_data()) {
                        // two responses should not deliver the same item.
                        body().reads(index).set_data(resp.reads(i).data());
                    }
                } else if(resp.reads(i).has_return_status()) {
                    body().reads(index).set_return_status(resp.reads(i).return_status());
                } else {
                    body().reads(index).set_return_status(Protocol::StorageElement::KEY_MISSING);
                }
            }
        }
        bool hasall = true;
        for (int i = 0; i < body().reads_size(); i++) {
            if (!body().reads(i).has_data() && !body().reads(i).has_return_status()) {
                SILOG(persistence,info,"Body "<<i<<" has no data or return status");
                hasall = false;
                break;
            }
        }
        if (hasall) {
            SILOG(persistence,insane,"Got a whole persistence message!");
            mRealCallback(this, responseHeader, Protocol::Response::SUCCESS);
        } else {
            SILOG(persistence,debug,"Will keep waiting for rest of persistence message ...");
        }
    }
//do not call
    void setCallback(const SentMessage::QueryCallback &cb) {
        this->SentMessage::setCallback(cb);
    }

public:
    PersistenceSentMessage(QueryTracker *tracker) : SuperClass(tracker) {
        SentMessage::setCallback(&ThisClass::receivedPersistenceMessageStatic);
    }
    ~PersistenceSentMessage() {}
    
    /// sets the callback handler. Must be called at least once.
    void setPersistenceCallback(const PersistenceCallback &cb) {
        mRealCallback = cb;
    }


    using SuperClass::setTimeout;
    void serializeSend() {
        for (int i = 0; i < body().reads_size(); i++) {
            body().reads(i).set_index(i);
            body().reads(i).clear_data();
            body().reads(i).clear_return_status();
        }
        SuperClass::serializeSend();
    }
    using SuperClass::header;
    using SuperClass::body;
};

typedef PersistenceSentMessage<Protocol::ReadWriteSet> SentReadWriteSet;
typedef PersistenceSentMessage<Protocol::Minitransaction> SentTransaction;

}
}
#endif

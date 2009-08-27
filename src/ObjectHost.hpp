/*  cbr
 *  ObjectHost.hpp
 *
 *  Copyright (c) 2009, Ewen Cheslack-Postava
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
 *  * Neither the name of cbr nor the names of its contributors may
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

#ifndef _CBR_OBJECT_HOST_HPP_
#define _CBR_OBJECT_HOST_HPP_

#include "Utility.hpp"
#include "Message.hpp"

namespace CBR {

class Object;
class ObjectFactory;
class ObjectHost;
class Trace;
class Server;
class ObjectConnection;

class ObjectHostContext {
public:
    ObjectHostContext()
     : objectHost(NULL),
       objectFactory(NULL),
       lastTime(Time::null()),
       time(Time::null()),
       trace(NULL)
    {
    }

    ObjectHost* objectHost;
    ObjectFactory* objectFactory;
    Time lastTime;
    Time time;
    Trace* trace;
}; // class ObjectHostContext

class ObjectHost {
public:
    // FIXME the ServerID is used to track unique sources, we need to do this separately for object hosts
    ObjectHost(const ServerID& sid, ObjectFactory* obj_factory, Trace* trace);
    ~ObjectHost();

    const ObjectHostContext* context() const;

    void openConnection(ObjectConnection* conn);

    // FIXME should not be infinite queue and should report push error
    bool send(const Object* src, const uint16 src_port, const UUID& dest, const uint16 dest_port, const std::string& payload);

    void tick(const Time& t);

    // FIXME this is temporary until we have messages going out over
    // a real network connection to the server, which should then put
    // them on its ObjectMessageQueue
    void setServer(Server* server);

private:
    ObjectHostContext* mContext;
    uint64 mOHId;
    Server* mServer;
    std::queue<std::string*> mOutgoingQueue;
}; // class ObjectHost

} // namespace CBR


#endif //_CBR_OBJECT_HOST_HPP_

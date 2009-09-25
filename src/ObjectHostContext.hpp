/*  cbr
 *  ObjectHostContext.hpp
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

#ifndef _CBR_OBJECT_HOST_CONTEXT_HPP_
#define _CBR_OBJECT_HOST_CONTEXT_HPP_

#include "Utility.hpp"
#include "Message.hpp"
#include "Timer.hpp"

namespace CBR {

class ObjectFactory;
class ObjectHost;
class Trace;

typedef uint64 ObjectHostID;

class ObjectHostContext {
public:
    ObjectHostContext(ObjectHostID _id, const Time& epoch, const Time& curtime)
     : id(_id),
       objectHost(NULL),
       objectFactory(NULL),
       lastTime(curtime),
       time(curtime),
       trace(NULL),
       mEpoch(epoch)
    {
    }


    Time epoch() const {
        return mEpoch.read();
    }
    Duration sinceEpoch(const Time& rawtime) const {
        return rawtime - mEpoch.read();
    }
    Time simTime(const Duration& sinceStart) const {
        return Time::null() + sinceStart;
    }
    Time simTime(const Time& rawTime) const {
        return simTime( sinceEpoch(rawTime) );
    }
    // WARNING: The evaluates Timer::now, which shouldn't be done too often
    Time simTime() const {
        return simTime( Timer::now() );
    }


    ObjectHostID id;
    ObjectHost* objectHost;
    ObjectFactory* objectFactory;
    Time lastTime;
    Time time;
    Trace* trace;
private:
    Sirikata::AtomicValue<Time> mEpoch;
}; // class ObjectHostContext

} // namespace CBR


#endif //_CBR_OBJECT_HOST_CONTEXT_HPP_

/*  Sirikata
 *  SerializationCheck.hpp
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

#ifndef _SIRIKATA_SERIALIZATION_CHECK_HPP_
#define _SIRIKATA_SERIALIZATION_CHECK_HPP_

#include "util/Platform.hpp"
#include <boost/thread/thread.hpp>

namespace Sirikata {

/** Verifies that code blocks you expect to be handled in a serialized
 *  fashion (i.e. non-concurrently) are actually handled that way.
 *  Mark these sections with calls to serializedEnter() and
 *  serializedExit() at the beinning and end respectively and those
 *  calls will assert in debug mode if this condition is violated.
 *  This can be used recursively within a thread -- multiple calls
 *  to serializedEnter() from the same thread will not result in
 *  an error.
 */
class SerializationCheck {
public:
    // Used to mark an entire scope as
    class Scoped {
    public:
        Scoped(SerializationCheck* p)
         : parent(p)
        {
            parent->serializedEnter();
        }

        ~Scoped() {
            parent->serializedExit();
        }
    private:
        Scoped();
        Scoped(Scoped& rhs);
        Scoped& operator=(Scoped& rhs);

        SerializationCheck* parent;
    };



    SerializationCheck()
     : mAccessors(0),
       mThreadID()
    {
    }

    inline void serializedEnter() {
        int32 val = ++mAccessors;

        assert(val >= 1);

        if (val == 1) {
            // We're the first ones in here, we need to setup the thread id
            mThreadID = boost::this_thread::get_id();
        }
        else {
            // We got in here later on.  This is only valid if we were in the
            // same thread, so the thread had better already be marked and
            // match ours
            assert(mThreadID == boost::this_thread::get_id());
        }
    }

    inline void serializedExit() {
        if (mAccessors == (int32)1) {
            // We should be the only one left accessing this and a
            // serializedEnter should *not* be getting called, so we
            // can erase the thread ID now.
            mThreadID = boost::thread::id();
        }

        int32 val = --mAccessors;
        assert(val >= 0);
    }
private:
    // Implementation Note: Technically this isn't all thread safe.  However,
    // the core component which tracks the number of accessors and their order
    // off access is thread safe. Any thread-unsafe accesses should only result
    // in a bad comparison, which should result in an assertion anyway since the
    // whole point of this class is to verify that multiple threads are not trying
    // to access shared data (including this object) at the same time.
    AtomicValue<int32> mAccessors;
    boost::thread::id mThreadID;
};

} // namespace Sirikata

#endif //_SIRIKATA_SERIALIZATION_CHECK_HPP_

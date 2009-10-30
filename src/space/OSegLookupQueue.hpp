/*  cbr
 *  OSegLookupQueue.hpp
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

#ifndef _OSEG_LOOKUP_QUEUE_HPP_
#define _OSEG_LOOKUP_QUEUE_HPP_

#include "Utility.hpp"
#include "Message.hpp"
#include "ObjectSegmentation.hpp"

namespace CBR {

/** OSegLookupQueue manages outstanding OSeg lookups.  Lookups are submitted
 *  and either accepted and we commit to finishing them or rejected immediately.
 *  The user can specify a policy for how these rejections occur, e.g. based
 *  on a total number of outstanding lookups, a total number of bytes in messages
 *  for outstanding lookups, etc.
 */
class OSegLookupQueue : public OSegListener {
public:
    /** Predicate which determines whether a message will be admitted for a destination object lookup.
     *  \param object the object the lookup is being performed on
     *  \param msg_size the size of the object submitted with the request
     *  \param cur_tot_size the current total size of messages in the queue
     */
    typedef std::tr1::function<bool(UUID, size_t, size_t)> PushPredicate;
    /** Callback type for lookups, taking the message the lookup was performed on and the
     *  ServerID the OSeg returned. If you need additional information it must be curried via bind().
     */
    typedef std::tr1::function<void(CBR::Protocol::Object::ObjectMessage*, ServerID)> LookupCallback;

private:
    struct OSegLookup {
        CBR::Protocol::Object::ObjectMessage* msg;
        LookupCallback cb;
    };

    /** A normal vector of OSegLookups except it also maintains the
     *  total size of all its elements.
     */
    class OSegLookupList : protected std::vector<OSegLookup> {
        typedef std::vector<OSegLookup> OSegLookupVector;
        size_t mTotalSize;
    public:
        size_t ByteSize() const;
        size_t size() const;
        OSegLookup& operator[] (size_t where);
        void push_back(const OSegLookup& lu);
        OSegLookupVector::iterator begin();
        OSegLookupVector::iterator end();
    };

    typedef std::tr1::unordered_map<UUID, OSegLookupList, UUID::Hasher> LookupMap;


    IOStrand* mNetworkStrand;
    ObjectSegmentation* mOSeg; // The OSeg that does the heavy lifting

    LookupMap mLookups; // Map of object id being queried -> msgs destined for that object
    PushPredicate mPredicate; // Predicate determining entry to the queue
    int32 mTotalSize;

    /* OSegListener Interface */
    virtual void osegLookupCompleted(const UUID& id, const ServerID& dest);
    /* Main thread handler for lookups. */
    void handleLookupCompleted(const UUID& id, const ServerID& dest);
public:
    /** Create an OSegLookupQueue which uses the specified ObjectSegmentation to resolve queries and
     *  the specified predicate to determine if new lookups are accepted.
     *  \param net_strand the strand used for networking, i.e. the one which should handle lookup
     *                    results
     *  \param oseg the ObjectSegmentation which resolves queries
     *  \param pred the predicate which controls whether new lookups are admitted
     */
    OSegLookupQueue(IOStrand* net_strand, ObjectSegmentation* oseg, const PushPredicate& pred);

    /** Perform an OSeg lookup, calling the specified callback when the result is available.
     *  If the result is available immediately, the callback may be triggered during this
     *  call.  Otherwise, it will be triggered when a service() call produces a result.
     *  Note that if the request is accepted, the message is owned by the OSegLookupQueue until
     *  the callback is invoked, at which time control is passed back to the caller.
     *  \param msg the ObjectMessage to perform the lookup for
     *  \param cb the callback to invoke when the lookup is complete
     *  \returns true if the lookup was accepted, false if it was rejected (due to the push predicate).
     */
    bool lookup(CBR::Protocol::Object::ObjectMessage* msg, const LookupCallback& cb);
};

} // namespace CBR

#endif //_OSEG_LOOKUP_QUEUE_HPP_

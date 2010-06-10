/*  Sirikata
 *  LocalForwarder.hpp
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

#ifndef _SIRIKATA_LOCAL_FORWARDER_HPP_
#define _SIRIKATA_LOCAL_FORWARDER_HPP_

#include <sirikata/cbrcore/Utility.hpp>
#include "ObjectConnection.hpp"

namespace Sirikata {

/** LocalForwarder maintains a map of objects that are directly connected to
 *  this space server. It operates in the same strand as the networking to
 *  allow very fast forwarding of messages between objects connected to the same
 *  space server.
 */
class LocalForwarder {
  public:
    /** Create a LocalForwarder.
     *  \param ctx SpaceContext for this LocalForwarder to operate in
     */
    LocalForwarder(SpaceContext* ctx);

    /** Notify the LocalForwarder that a new object connection is now
     *  available.  This transfers ownership of the ObjectConnection to the
     *  LocalForwarder.
     *  \param conn the new connection to add
     */
    void addActiveConnection(ObjectConnection* conn);

    /** Remove the connection for an object.
     *  \param objid the UUID of the object to remove
     */
    void removeActiveConnection(const UUID& objid);

    /** Try to forward a message directly, shortcutting any forwarding
     *  code.  If forwarded, the LocalForwarder retains ownership of
     *  the message.
     *  \param msg the message to try to forward
     *  \returns true if the message was forwarded, false otherwise
     */
    bool tryForward(Sirikata::Protocol::Object::ObjectMessage* msg);
  private:
    typedef std::tr1::unordered_map<UUID, ObjectConnection*, UUID::Hasher> ObjectConnectionMap;

    SpaceContext* mContext;
    ObjectConnectionMap mActiveConnections;
    boost::mutex mMutex;
};

} // namespace Sirikata

#endif //_SIRIKATA_LOCAL_FORWARDER_HPP_

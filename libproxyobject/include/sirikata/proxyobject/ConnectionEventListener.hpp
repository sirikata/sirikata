/*  Sirikata Object Host
 *  ConnectionEvent.hpp
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
#ifndef _SIRIKATA_CONNECTION_EVENT_LISTENER_HPP_
#define _SIRIKATA_CONNECTION_EVENT_LISTENER_HPP_

namespace Sirikata {

/** ConnectionEventListener listens for events relating to object host
 * connections. This is useful for monitoring the health of the underlying
 * system without directly exposing the details.
 *
 * FIXME This should be in liboh, but it is primarily useful to display plugins,
 * e.g. Ogre, which currently only use libproxyobject.
 */
class SIRIKATA_PROXYOBJECT_EXPORT ConnectionEventListener {
public:
    virtual ~ConnectionEventListener(){}

    /** Invoked upon connection.
     *  \param addr the address connected to
     */
    virtual void onConnected(const Network::Address& addr) {};
    /** Invoked upon disconnection.
     *  \param addr the address disconnected from
     *  \param requested indicates whether the user requested the disconnection
     *  \param reason if requested is false, gives a textual description of the
     *  reason for the disconnection
     */
    virtual void onDisconnected(const Network::Address& addr, bool requested, const String& reason) {};
};

} // namespace Sirikata

#endif //_SIRIKATA_CONNECTION_EVENT_LISTENER_HPP_

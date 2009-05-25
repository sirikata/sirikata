/*  Sirikata Proximity Management -- Introduction Services
 *  ProximitySystem.hpp
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
#ifndef _SPACE_SPACEPLUGIN_HPP_
#define _SPACE_SPACEPLUGIN_HPP_
namespace Sirikata {
class ObjectReference;
}
namespace Sirikata {
class RoutableMessageHeader;
}
namespace Sirikata { namespace Space {
class SpacePlugin {
public:
    virtual ~SpacePlugin(){}
    /**
     * Registers that another space plugin is interested in messages accepted/produced by this plugin
     * For instance, the NewObject plugin is probably going to get registrations of interest from just 'bout anyone
     * And those plugins would like the RetObj messages
     * Whereas the Loc plugin is probably going to get registrations of interest from anyone and may just care about vanilla incoming messages
     */
    virtual void forwardMessagesTo(SpacePlugin*)=0;
    /**
     * Process a message that may be meant for the space system
     */
    virtual void processOpaqueSpaceMessage(const ObjectReference*object,
                                           const RoutableMessageHeader&,
                                           const void *serializedMessageBody,
                                           size_t serializedMessageBodySize)=0;

    /**
     * Process a message that may be meant for the space system
     */
    virtual void processOpaqueSpaceMessage(const ObjectReference*object,
                                           const void *serializedMessage,
                                           size_t serializedMessageSize)=0;

};

} }
#endif

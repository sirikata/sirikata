/*  Sirikata Object Host
 *  MeshListener.hpp
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
#ifndef _SIRIKATA_MESH_LISTENER_HPP_
#define _SIRIKATA_MESH_LISTENER_HPP_

#include <sirikata/core/transfer/URI.hpp>
#include <sirikata/mesh/Meshdata.hpp>
#include <sirikata/proxyobject/ProxyObject.hpp>

namespace Sirikata {


class SIRIKATA_PROXYOBJECT_EXPORT MeshListener
{
    public:
        virtual ~MeshListener() {}

        virtual void onSetMesh (ProxyObjectPtr proxy, Transfer::URI const& newMesh, const SpaceObjectReference& sporef) = 0;
        virtual void onSetScale (ProxyObjectPtr proxy, float32 newScale,const SpaceObjectReference& sporef ) = 0;
        virtual void onSetPhysics (ProxyObjectPtr proxy, const String& phy,const SpaceObjectReference& sporef ) {};
};

} // namespace Sirikata

#endif

/*  Sirikata Utilities -- Sirikata Listener Pattern
 *  ProxyCameraObject.hpp
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

#ifndef _SIRIKATA_PROXY_MESH_OBJECT_HPP_
#define _SIRIKATA_PROXY_MESH_OBJECT_HPP_
#include "MeshListener.hpp"
#include "ProxyPositionObject.hpp"
namespace Sirikata {

typedef Provider<MeshListener*> MeshProvider;

/** Represents any object with an attached mesh. */
class SIRIKATA_OH_EXPORT ProxyMeshObject
  : public MeshProvider,
    public ProxyPositionObject {
protected:
    URI mMeshURI;
    Vector3f mScale;
    bool mPhysical;
public:
    ProxyMeshObject(ProxyManager *man, const SpaceObjectReference&id);
    void setMesh (const URI &newMesh);
    void setScale (const Vector3f &newScale);
    void setPhysical (const int &mode);

    inline const URI & getMesh() const {
        return mMeshURI;
    }
    inline const Vector3f &getScale() const {
        return mScale;
    }
    inline const bool getPhysical() const {
        return mPhysical;
    }
    };
}
#endif

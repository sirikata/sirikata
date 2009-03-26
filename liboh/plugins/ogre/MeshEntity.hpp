/*  Sirikata Graphical Object Host
 *  MeshObject.hpp
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
#ifndef SIRIKATA_GRAPHICS_MESHOBJECT_HPP__
#define SIRIKATA_GRAPHICS_MESHOBJECT_HPP__

#include <oh/Platform.hpp>

#include "options/Options.hpp"
#include "OgreSystem.hpp"
#include "OgrePlugin.hpp"

#include <oh/ProxyMeshObject.hpp>
#include <oh/MeshListener.hpp>
#include "Entity.hpp"
#include <OgreEntity.h>

namespace Sirikata {
namespace Graphics {

class MeshEntity
    : public Entity,
      public MeshListener {

    URI mMeshURI;


    void created(const Ogre::MeshPtr &mesh);

    Ogre::Entity *getOgreEntity() const {
        return static_cast<Ogre::Entity*const>(mOgreObject);
    }

public:
    ProxyMeshObject &getProxy() const {
        return *std::tr1::static_pointer_cast<ProxyMeshObject>(mProxy);
    }
    MeshEntity(OgreSystem *scene,
               const std::tr1::shared_ptr<ProxyMeshObject> &pmo,
               const UUID &id);

    virtual ~MeshEntity();

    void meshChanged(const URI&meshFile);

    Vector3f getScale() const {
        return Vector3f(0,0,0);//fromOgre(getOgreEntity()->getScale());
    }
    void setScale(const Vector3f &scale) {
        mSceneNode->setScale(toOgre(scale));
    }

  /*
    virtual bool loadMesh(const String&name){
        return false;
    }
  */
};

}
}

#endif

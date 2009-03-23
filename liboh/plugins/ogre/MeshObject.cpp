/*  Sirikata Graphical Object Host
 *  MeshObject.cpp
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
#include "MeshObject.hpp"
#include <OgreMeshManager.h>
#include <OgreResourceGroupManager.h>
namespace Sirikata {
namespace Graphics {

MeshObject::MeshObject(OgreSystem *scene,
               const std::tr1::shared_ptr<ProxyMeshObject> &pmo,
               const UUID &id)
        : Entity(scene,
                 pmo,
                 id,
                 scene->getSceneManager()->createEntity(id.readableHexData(), Ogre::SceneManager::PT_CUBE))
{}

void MeshObject::meshChanged(const URI &meshFile) {
    mMeshURI = meshFile;
    //scene->getDependencyManager()->loadMesh(id, meshFile, std::tr1::bind(&MeshObject::created, this, _1));
    Ogre::MeshPtr ogreMesh = Ogre::MeshManager::getSingleton().load(meshFile.filename(), Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
    created(ogreMesh);
}

void MeshObject::created(const Ogre::MeshPtr &mesh) {
    getScene()->getSceneManager()->destroyEntity(id().readableHexData());
    Ogre::MovableObject *meshObj = getScene()->getSceneManager()->createEntity(id().readableHexData(),
                                                                               mesh->getName());
    init(meshObj);
}

MeshObject::~MeshObject() {
    init(NULL);
    getScene()->getSceneManager()->destroyEntity(mId.readableHexData());
    getProxy().removeListener(this);
}

}
}

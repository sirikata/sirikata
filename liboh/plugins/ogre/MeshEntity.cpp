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
#include <oh/Platform.hpp>
#include "MeshEntity.hpp"
#include <util/AtomicTypes.hpp>
#include <OgreMeshManager.h>
#include <OgreResourceGroupManager.h>
#include <OgreSubEntity.h>
#include <OgreEntity.h>
#include "resourceManager/GraphicsResourceManager.hpp"
namespace Sirikata {
namespace Graphics {

MeshEntity::MeshEntity(OgreSystem *scene,
                       const std::tr1::shared_ptr<ProxyMeshObject> &pmo,
                       const std::string &id)
        : Entity(scene,
                 pmo,
                 id.length()?id:ogreMeshName(pmo->getObjectReference()),
                 NULL)
{
    getProxy().MeshProvider::addListener(this);
    Meru::GraphicsResourceManager* grm = Meru::GraphicsResourceManager::getSingletonPtr();
    mResource = std::tr1::dynamic_pointer_cast<Meru::GraphicsResourceEntity>
        (grm->getResourceEntity(pmo->getObjectReference(), this));
    unloadMesh();
}

void MeshEntity::meshChanged(const URI &meshFile) {
    mMeshURI = meshFile;

    Meru::GraphicsResourceManager* grm = Meru::GraphicsResourceManager::getSingletonPtr();
    Meru::SharedResourcePtr newMeshPtr = grm->getResourceAsset(meshFile, Meru::GraphicsResource::MESH);
    mResource->setMeshResource(newMeshPtr);
}

std::string MeshEntity::ogreMeshName(const SpaceObjectReference&ref) {
    return "Mesh:"+ref.toString();
}
std::string MeshEntity::ogreMovableName()const{
    return ogreMeshName(id());
}
MeshEntity::~MeshEntity() {
    mResource->entityDestroyed();
    Ogre::Entity * toDestroy=getOgreEntity();
    init(NULL);
    if (toDestroy) {
        getScene()->getSceneManager()->destroyEntity(toDestroy);
    }
    getProxy().MeshProvider::removeListener(this);

}

void MeshEntity::loadMesh(const String& meshname)
{

    Ogre::Entity * oldMeshObj=getOgreEntity();

    /** FIXME we need a better way of generating unique id's. We should
     *  be able to use just the uuid, but its not enough since we want
     *  to be able to create a replacement entity before destroying the
     *  original.  Also, this is definitely thread safe.
     */
    static AtomicValue<int32> idx ((int32)0);
    Ogre::Entity* new_entity;
    try {
      try {
        new_entity = getScene()->getSceneManager()->createEntity(
            ogreMovableName(), meshname);
      } catch (Ogre::InvalidParametersException &e) {
        SILOG(ogre,error,"Got invalid parameters");
        throw;
      }
    } catch (...) {
        SILOG(ogre,error,"Failed to load mesh "<<mMeshURI<< " (id "<<id()<<")!");
        new_entity = getScene()->getSceneManager()->createEntity(ogreMovableName(),Ogre::SceneManager::PT_CUBE);
        /*
        init(NULL);
        if (oldMeshObj) {
            getScene()->getSceneManager()->destroyEntity(oldMeshObj);
        }
        return;
        */
    }
    SILOG(ogre,debug,"Bounding box: " << new_entity->getBoundingBox());
    if (false) { //programOptions[OPTION_ENABLE_TEXTURES].as<bool>() == false) {
        new_entity->setMaterialName("BaseWhiteTexture");
    }
    unsigned int num_subentities=new_entity->getNumSubEntities();
    SHA256 random_seed=SHA256::computeDigest(id().toRawHexData());
    const SHA256::Digest &digest=random_seed.rawData();
    Ogre::Vector4 parallax_steps(getScene()->mParallaxSteps->as<float>(),getScene()->mParallaxShadowSteps->as<int>(),0.0,1.0);
    unsigned short av=digest[0]+256*(unsigned short)digest[1];
    unsigned short bv=digest[2]+256*(unsigned short)digest[3];
    unsigned short cv=digest[4]+256*(unsigned short)digest[5];
    unsigned short dv=digest[6]+256*(unsigned short)digest[7];
    unsigned short ev=digest[8]+256*(unsigned short)digest[9];
    unsigned short fv=digest[10]+256*(unsigned short)digest[11];
    Ogre::Vector4 random_values((float)av/65535.0f,(float)bv/65535.0f,(float)cv/65535.0f,1.0);
    random_values.x=random_values.x*.0+1.0;//fixme: make this per object
    random_values.y=random_values.y*.0+1.0;
    random_values.z=random_values.z*.0+1.0;
    for (unsigned int subent=0;subent<num_subentities;++subent) {
        new_entity->getSubEntity(subent)->setCustomParameter(0,random_values);
        new_entity->getSubEntity(subent)->setCustomParameter(1,parallax_steps);
    }

    init(new_entity);
    if (oldMeshObj) {
        getScene()->getSceneManager()->destroyEntity(oldMeshObj);
    }
}

void MeshEntity::unloadMesh() {
    Ogre::Entity * meshObj=getOgreEntity();
    //init(getScene()->getSceneManager()->createEntity(ogreMovableName(), Ogre::SceneManager::PT_CUBE));
    init(NULL);
    if (meshObj) {
        getScene()->getSceneManager()->destroyEntity(meshObj);
    }
}


}
}

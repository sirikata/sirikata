/*  Sirikata
 *  Entity.hpp
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
#ifndef SIRIKATA_OGRE_ENTITY_HPP__
#define SIRIKATA_OGRE_ENTITY_HPP__

#include <sirikata/ogre/Platform.hpp>
#include <OgreMovableObject.h>
#include <OgreRenderable.h>
#include <OgreSceneManager.h>
#include <sirikata/ogre/OgreConversions.hpp>
#include <sirikata/core/network/IOTimer.hpp>
#include <sirikata/core/transfer/URI.hpp>
#include <sirikata/core/transfer/TransferData.hpp>

#include <sirikata/ogre/resourceManager/AssetDownloadTask.hpp>

#include <sirikata/core/util/ListenerProvider.hpp>
#include <sirikata/core/util/Liveness.hpp>

namespace Sirikata {
namespace Graphics {

class OgreRenderer;
class Entity;

class SIRIKATA_OGRE_EXPORT EntityListener {
  public:
    virtual ~EntityListener();

    virtual void entityLoaded(Entity* ent, bool success) {}
};

/** Base class for objects represented in Ogre as a mesh. */
class SIRIKATA_OGRE_EXPORT Entity : public Provider<EntityListener*>, public Liveness {
public:
    typedef std::map<int, std::pair<String, Ogre::MaterialPtr> > ReplacedMaterialMap;
    typedef std::map<String, String > TextureBindingsMap;
protected:
    OgreRenderer *const mScene;
    String mName;

    Ogre::Entity* mOgreObject;
    Ogre::SceneNode *mSceneNode;

    std::list<Entity*>::iterator mMovingIter;

    ReplacedMaterialMap mReplacedMaterials;
    TextureBindingsMap mTextureBindings;

    std::tr1::shared_ptr<TextureBindingsMap> mTextureFingerprints;

    typedef std::vector<Ogre::Light*> LightList;
    LightList mLights;

    Transfer::URI mURI;
    String mURIString;

    bool mActiveCDNArchive;
    unsigned int mCDNArchive;

    // We need to track this because Ogre doesn't seem to do the right
    // thing if you toggle visibility with cascading, then later add
    // more nodes.
    bool mVisible;
    // And we track this because it's more efficient than looping through all
    // available animations on the mesh.
    Ogre::AnimationState* mCurrentAnimation;

    AssetDownloadTaskPtr mAssetDownload;

    void fixTextures();

    void createMesh(Liveness::Token alive);

    void init(Ogre::Entity *obj);

    // Note that this is forceful -- it doesn't do any checks. Generally you
    // should only use it from within checkDynamic().
    void setDynamic(bool isStatic);
    // Checks whether the object is dynamic and updates the renderer
    void checkDynamic();

    void updateScale(float scale);
    void updateVisibility();

public:
    Entity(OgreRenderer *scene, const String& name);
    virtual ~Entity();

    // These should be overridden to allow this class to learn about the
    // properties of this Entity or override the basic behavior of Entity.
    virtual BoundingSphere3f bounds() = 0;
    virtual float32 priority() = 0;
    virtual void tick(const Time& t, const Duration& deltaTime);
    // Check if the object is dynamic, i.e. needs updates every frame. The
    // Entity base class is only non-static during animations. Be sure to
    // include the base classes implementation when checking whether the object
    // is static.
    virtual bool isDynamic() const;

    static Entity *fromMovableObject(Ogre::MovableObject *obj);

    void removeFromScene();
    void addToScene(Ogre::SceneNode *newParent=NULL);

    OgreRenderer *getScene() const{
        return mScene;
    }

    Ogre::SceneNode *getSceneNode() {
        return mSceneNode;
    }

    Ogre::Entity *getOgreEntity() const {
        return mOgreObject;
    }

    void setOgrePosition(const Vector3d &pos);
    void setOgreOrientation(const Quaternion &orient);

    Vector3d getOgrePosition();
    Quaternion getOgreOrientation();

    void setSelected(bool selected);

    static std::string ogreMeshName(const String& name);
    std::string ogreMovableName()const;

    const String& id()const{
        return mName;
    }

    void setVisible(bool vis);

    void setAnimation(const String& name);

    void bindTexture(const std::string &textureName, const String& objId);
    void unbindTexture(const std::string &textureName);

    void processMesh(Transfer::URI const& newMesh);

    /** Load the mesh and use it for this entity
     *  \param meshname the name (ID) of the mesh to use for this entity
     */
    void loadMesh(const String& meshname);

    void unloadMesh();


    protected:

    // If a mesh had already been downloaded (by us or someone else), we have
    // the hash, *and* the object still exists, we should be able to just add it
    // to the scene.
    bool tryInstantiateExistingMesh(const String& meshname);
};
typedef std::tr1::shared_ptr<Entity> EntityPtr;

}
}

#endif //SIRIKATA_OGRE_ENTITY_HPP__

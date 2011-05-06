/*  Sirikata Graphical Object Host
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
#ifndef SIRIKATA_GRAPHICS_ENTITY_HPP__
#define SIRIKATA_GRAPHICS_ENTITY_HPP__

#include <sirikata/core/util/UUID.hpp>
#include <sirikata/proxyobject/ProxyObject.hpp>
#include <OgreMovableObject.h>
#include <OgreRenderable.h>
#include <OgreSceneManager.h>
#include "OgreConversions.hpp"
#include <sirikata/core/network/IOTimer.hpp>
#include <sirikata/core/transfer/URI.hpp>
#include <sirikata/core/transfer/TransferData.hpp>
namespace Sirikata {
namespace Graphics {
class OgreSystem;

/** Base class for any ProxyObject that has a representation in Ogre. */
class Entity
  : public PositionListener,
    public ProxyObjectListener,
    public MeshListener
{
public:
    typedef std::map<int, std::pair<String, Ogre::MaterialPtr> > ReplacedMaterialMap;
    typedef std::map<String, String > TextureBindingsMap;
protected:
    OgreSystem *const mScene;
    const ProxyObjectPtr mProxy;

    Ogre::Entity* mOgreObject;
    Ogre::SceneNode *mSceneNode;

    std::list<Entity*>::iterator mMovingIter;

    ReplacedMaterialMap mReplacedMaterials;
    TextureBindingsMap mTextureBindings;

    uint32 mRemainingDownloads; // Downloads remaining before loading can occur
    std::tr1::shared_ptr<TextureBindingsMap> mTextureFingerprints;

    typedef std::vector<Ogre::Light*> LightList;
    LightList mLights;

    Transfer::URI mURI;
    String mURIString;

    bool mHaveURIHash;
    SHA256 mURIHash; // Hash of mURI content. Stored so we can possibly avoid a download.

    bool mActiveCDNArchive;
    unsigned int mCDNArchive;

    Network::IOTimerPtr mDestroyTimer;

    // We need to track this because Ogre doesn't seem to do the right
    // thing if you toggle visibility with cascading, then later add
    // more nodes.
    bool mVisible;

    void fixTextures();

    // Wrapper for createMesh which allows us to use a WorkQueue
    void createMeshWork(Mesh::MeshdataPtr md);

    void createMesh(Mesh::MeshdataPtr md);

    void init(Ogre::Entity *obj);

    void setStatic(bool isStatic);

    void updateScale(float scale);
    void updateVisibility();

    void handleDestroyTimeout();
protected:
    void setOgrePosition(const Vector3d &pos);

    void setOgreOrientation(const Quaternion &orient);
public:
    ProxyObject &getProxy() const {
        return *mProxy;
    }
    const ProxyObjectPtr &getProxyPtr() const {
        return mProxy;
    }
    Entity(OgreSystem *scene,
        const ProxyObjectPtr &ppo);

    ~Entity();

    static Entity *fromMovableObject(Ogre::MovableObject *obj);

    void removeFromScene();
    void addToScene(Ogre::SceneNode *newParent=NULL);

    OgreSystem *getScene() const{
        return mScene;
    }

    void updateLocation(const TimedMotionVector3f &newLocation, const TimedMotionQuaternion& newOrient, const BoundingSphere3f& newBounds);

    // ProxyObjectListener
    virtual void validated();
    virtual void invalidated();
    virtual void destroyed();

    Ogre::SceneNode *getSceneNode() {
        return mSceneNode;
    }

    Ogre::Entity *getOgreEntity() const {
        return mOgreObject;
    }

    Vector3d getOgrePosition();

    Quaternion getOgreOrientation();

    void extrapolateLocation(TemporalValue<Location>::Time current);

    void setSelected(bool selected);

    static std::string ogreMeshName(const SpaceObjectReference&ref);
    std::string ogreMovableName()const;


    const SpaceObjectReference&id()const{
        return mProxy->getObjectReference();
    }

    void setVisible(bool vis);

    void bindTexture(const std::string &textureName, const SpaceObjectReference &objId);
    void unbindTexture(const std::string &textureName);

    void processMesh(Transfer::URI const& newMesh);

    void downloadFinished(std::tr1::shared_ptr<Transfer::ChunkRequest> request,
        std::tr1::shared_ptr<const Transfer::DenseData> response, Mesh::MeshdataPtr md);

    /** Load the mesh and use it for this entity
     *  \param meshname the name (ID) of the mesh to use for this entity
     */
    void loadMesh(const String& meshname);

    void unloadMesh();

    void downloadMeshFile(Transfer::URI const& uri);

    // interface from MeshListener
    public:
        virtual void onSetMesh (ProxyObjectPtr proxy, Transfer::URI const& newMesh);
        virtual void onSetScale (ProxyObjectPtr proxy, float32 newScale );
        virtual void onSetPhysical (ProxyObjectPtr proxy, PhysicalParameters const& pp );

    protected:

    void handleMeshParsed(Mesh::MeshdataPtr md);

    // If a mesh had already been downloaded (by us or someone else), we have
    // the hash, *and* the object still exists, we should be able to just add it
    // to the scene.
    bool tryInstantiateExistingMesh();

    void MeshDownloaded(std::tr1::shared_ptr<Transfer::ChunkRequest>request, std::tr1::shared_ptr<const Transfer::DenseData> response);

    // After a mesh is downloaded, try instantiating it from an existing mesh,
    // i.e. in case this URI/underlying hash has already been loaded.
    void tryInstantiateExistingMeshOrParse(Transfer::ChunkRequestPtr request, Transfer::DenseDataPtr response);
};
typedef std::tr1::shared_ptr<Entity> EntityPtr;

}
}

#endif

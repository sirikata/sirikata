/*  Sirikata
 *  Entity.cpp
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

#include <boost/lexical_cast.hpp>
#include <sirikata/ogre/Entity.hpp>
#include <sirikata/ogre/OgreRenderer.hpp>
#include <sirikata/ogre/resourceManager/CDNArchive.hpp>
#include <sirikata/ogre/OgreHeaders.hpp>
#include <sirikata/ogre/Lights.hpp>
#include <sirikata/core/network/IOStrandImpl.hpp>
#include <sirikata/core/transfer/URL.hpp>
#include <sirikata/ogre/Util.hpp>
#include <sirikata/ogre/WebViewManager.hpp>
#include <FreeImage.h>
#include "ManualMaterialLoader.hpp"
#include "ManualSkeletonLoader.hpp"
#include "ManualMeshLoader.hpp"

#undef nil

using namespace Sirikata::Transfer;

namespace Sirikata {
namespace Graphics {

using namespace Sirikata::Mesh;


EntityListener::~EntityListener() {
}


static void fixOgreURI(String &uri) {
    for (String::iterator i=uri.begin();i!=uri.end();++i) {
        if(*i=='.') *i='{';
    }
}

class ReplaceTexture {
	const String &mFrom;
	const String &mTo;
public:
	ReplaceTexture(const String &from, const String &to)
		: mFrom(from),
		  mTo(to) {
	}
	bool operator()(Ogre::TextureUnitState *tus) {
		if (tus->getTextureName() == mFrom) {
			tus->setTextureName(mTo);
		}
		return false;
	}
};

class ShouldReplaceTexture {
    const TextureBindingsMap &mFrom;
public:
    ShouldReplaceTexture(const TextureBindingsMap &from)
     : mFrom(from)
    {
    }
    bool operator()(Ogre::TextureUnitState *tus) {
        if (mFrom.find(tus->getTextureName()) != mFrom.end()) {
            return true;
        }
        return false;
    }
};

template <class Functor>
bool forEachTexture(const Ogre::MaterialPtr &material, Functor func) {
	int numTechniques = material->getNumTechniques();
	for (int whichTechnique = 0; whichTechnique < numTechniques; whichTechnique++) {
		Ogre::Technique *tech = material->getTechnique(whichTechnique);
		int numPasses = tech->getNumPasses();
		for (int whichPass = 0; whichPass < numPasses; whichPass++) {
			Ogre::Pass *pass = tech->getPass(whichPass);
			int numTUS = pass->getNumTextureUnitStates();
			for (int whichTUS = 0; whichTUS < numTUS; whichTUS++) {
				Ogre::TextureUnitState *tus = pass->getTextureUnitState(whichTUS);
				if (func(tus)) {
					return true;
				}
			}
		}
	}
	return false;
}

Entity::Entity(OgreRenderer *scene, const String& name)
 : mScene(scene),
   mName(name),
   mOgreObject(NULL),
   mSceneNode(scene->getSceneManager()->createSceneNode( ogreMeshName(name) )),
   mMovingIter(scene->mMovingEntities.end()),
   mVisible(true),
   mCurrentAnimation(NULL),
   mInitialAnimationName(""),
   mMeshLoaded(false)
{
    mTextureFingerprints = TextureBindingsMapPtr(new TextureBindingsMap());

    mSceneNode->setInheritScale(false);
    addToScene(NULL);

    bool successful = scene->mSceneEntities.insert(
        OgreRenderer::SceneEntitiesMap::value_type(mName, this)).second;

    assert (successful == true);

    mCDNArchive=CDNArchiveFactory::getSingleton().addArchive();
    mActiveCDNArchive=true;

    unloadEntity();
}

Entity::~Entity() {
    Ogre::Entity * toDestroy=getOgreEntity();
    init(NULL);
    if (toDestroy) {
        getScene()->getSceneManager()->destroyEntity(toDestroy);

    }

    OgreRenderer::SceneEntitiesMap::iterator iter =
        mScene->mSceneEntities.find(mName);
    if (iter != mScene->mSceneEntities.end()) {
        // will fail while in the OgreRenderer destructor.
        mScene->mSceneEntities.erase(iter);
    }
    if (mMovingIter != mScene->mMovingEntities.end()) {
        mScene->mMovingEntities.erase(mMovingIter);
        mMovingIter = mScene->mMovingEntities.end();
    }

    removeFromScene();
    init(NULL);
    mSceneNode->detachAllObjects();
    /* detaches all children from the scene.
       There should be none, as the server should have adjusted their parents.
     */
    mSceneNode->removeAllChildren();
    mScene->getSceneManager()->destroySceneNode(mSceneNode);

    if (mActiveCDNArchive) {
        CDNArchiveFactory::getSingleton().removeArchive(mCDNArchive);
        mActiveCDNArchive=false;
    }
}

std::string Entity::ogreMeshName(const String& name) {
    return "Mesh:"+name;
}
std::string Entity::ogreMovableName()const{
    return ogreMeshName(id());
}

Entity *Entity::fromMovableObject(Ogre::MovableObject *movable) {
    return Ogre::any_cast<Entity*>(movable->getUserAny());
}

Ogre::Entity* Entity::getOgreEntity() const {
    return dynamic_cast<Ogre::Entity*>(mOgreObject);
}
Ogre::BillboardSet* Entity::getOgreBillboard() const {
    return dynamic_cast<Ogre::BillboardSet*>(mOgreObject);
}

void Entity::init(Ogre::MovableObject *obj) {
    if (mOgreObject) {
        mSceneNode->detachObject(mOgreObject);
    }
    mOgreObject = obj;
    if (obj) {
        mOgreObject->setUserAny(Ogre::Any(this));
        mSceneNode->attachObject(obj);
        updateScale( this->bounds().radius() );
        updateVisibility();
    }
}

void Entity::updateScale(float scale) {
    if (mSceneNode == NULL || mOgreObject == NULL) return;
    mSceneNode->setScale( 1.f, 1.f, 1.f );
    float rad = mOgreObject->getBoundingRadius();
    float rad_factor = scale / rad;
    mSceneNode->setScale( rad_factor, rad_factor, rad_factor );
}

void Entity::setDynamic(bool dyn) {
    const std::list<Entity*>::iterator end = mScene->mMovingEntities.end();
    if (!dyn) {
        if (mMovingIter != end) {
            SILOG(ogre,detailed,"Removed "<<this<<" from moving entities queue.");
            mScene->mMovingEntities.erase(mMovingIter);
            mMovingIter = end;
        }
    } else {
        if (mMovingIter == end) {
            SILOG(ogre,detailed,"Added "<<this<<" to moving entities queue.");
            mMovingIter = mScene->mMovingEntities.insert(end, this);
        }
    }
}

void Entity::checkDynamic() {
    bool isDyn = this->isDynamic();
    setDynamic(isDyn);
}

void Entity::setVisible(bool vis) {
    mVisible = vis;
    updateVisibility();
}

void Entity::setAnimation(const String& name) {
    // Disable current animation if we have one
    if (mCurrentAnimation) {
        mCurrentAnimation->setEnabled(false);
        mCurrentAnimation = NULL;
        setDynamic(isMobile());
    }

    if (name.empty()) return;

    if (mOgreObject == NULL || mMeshLoaded == false) {
      mInitialAnimationName = name;
      return;
    }

    if (mAnimationList.count(name) == 0) {
      return;
    }

    // Find and enable new animation
    Ogre::Entity* ent = getOgreEntity();
    if (ent == NULL)  {
        SILOG(ogre,error,"Tried to set animation on non-mesh.");
        return;
    }
    Ogre::AnimationState* state = ent->getAnimationState(name);
    if (state == NULL) {
        SILOG(ogre,error,"Tried to set animation to non-existant track.");
        return;
    }

    state->setEnabled(true);
    state->setLoop(true);
    state->setTimePosition(0.f);
    mCurrentAnimation = state;

    checkDynamic();
}

void Entity::updateVisibility() {
    mSceneNode->setVisible(mVisible, true);
}

void Entity::removeFromScene() {
    Ogre::SceneNode *oldParent = mSceneNode->getParentSceneNode();
    if (oldParent) {
        oldParent->removeChild(mSceneNode);
    }
    // Force dynamicity off
    setAnimation("");
    setDynamic(false);
}
void Entity::addToScene(Ogre::SceneNode *newParent) {
    if (newParent == NULL) {
        newParent = mScene->getSceneManager()->getRootSceneNode();
    }
    removeFromScene();
    newParent->addChild(mSceneNode);
    checkDynamic();
}

bool Entity::isDynamic() const {
    return (mCurrentAnimation != NULL);
}



void Entity::tick(const Time& t, const Duration& deltaTime) {
    if (mCurrentAnimation)
        mCurrentAnimation->addTime(deltaTime.seconds());

    checkDynamic();
}

void Entity::setOgrePosition(const Vector3d &pos) {
    //SILOG(ogre,detailed,"setOgrePosition "<<this<<" to "<<pos);
    Ogre::Vector3 ogrepos = toOgre(pos, getScene()->getOffset());
    const Ogre::Vector3 &scale = mSceneNode->getScale();
    mSceneNode->setPosition(ogrepos);
}

void Entity::setOgreOrientation(const Quaternion &orient) {
    //SILOG(ogre,detailed,"setOgreOrientation "<<this<<" to "<<orient);
    mSceneNode->setOrientation(toOgre(orient));
}

Vector3d Entity::getOgrePosition() {
    if (mScene == NULL) assert(false);
    return fromOgre(mSceneNode->getPosition(), mScene->getOffset());
}

Quaternion Entity::getOgreOrientation() {
    return fromOgre(mSceneNode->getOrientation());
}

void Entity::fixTextures() {
	Ogre::Entity *ent = getOgreEntity();
	if (!ent) {
		return;
	}
	SILOG(ogre,detailed,"Fixing texture for "<<id());
	int numSubEntities = ent->getNumSubEntities();
	for (ReplacedMaterialMap::iterator iter = mReplacedMaterials.begin();
			iter != mReplacedMaterials.end();
			++iter) {
		int whichSubEntity = iter->first;
		if (whichSubEntity >= numSubEntities) {
			SILOG(ogre,fatal,"Original material map not cleared when mesh changed. which = " << whichSubEntity << " num = " << numSubEntities);
			continue;
		}
		Ogre::SubEntity *subEnt = ent->getSubEntity(whichSubEntity);
		Ogre::MaterialPtr origMaterial = iter->second.second;
		subEnt->setMaterial(origMaterial);
		SILOG(ogre,detailed,"Resetting a material "<<id());
	}
	mReplacedMaterials.clear();
	for (int whichSubEntity = 0; whichSubEntity < numSubEntities; whichSubEntity++) {
		Ogre::SubEntity *subEnt = ent->getSubEntity(whichSubEntity);
		Ogre::MaterialPtr material = subEnt->getMaterial();
		if (forEachTexture(material, ShouldReplaceTexture(mTextureBindings))) {
			SILOG(ogre,detailed,"Replacing a material "<<id());
			Ogre::MaterialPtr newMaterial = material->clone(material->getName()+id(), false, Ogre::String());
			String newTexture;
			for (TextureBindingsMap::const_iterator iter = mTextureBindings.begin();
					iter != mTextureBindings.end();
					++iter) {
				SILOG(ogre,detailed,"Replacing a texture "<<id()<<" : "<<iter->first<<" -> "<<iter->second);
				forEachTexture(newMaterial, ReplaceTexture(iter->first, iter->second));
				newTexture = iter->second;
			}
			mReplacedMaterials.insert(
				ReplacedMaterialMap::value_type(whichSubEntity, std::pair<String, Ogre::MaterialPtr>(newTexture, newMaterial)));
			subEnt->setMaterial(newMaterial);
		}
	}
}

void Entity::bindTexture(const std::string &textureName, const String& objId) {
	mTextureBindings[textureName] = objId;
	fixTextures();
}
void Entity::unbindTexture(const std::string &textureName) {
	TextureBindingsMap::iterator iter = mTextureBindings.find(textureName);
	if (iter != mTextureBindings.end()) {
		mTextureBindings.erase(iter);
	}
	fixTextures();
}

void Entity::loadMesh(const String& meshname)
{
    Ogre::Entity* new_entity = NULL;
    try {
      try {
        new_entity = getScene()->getSceneManager()->createEntity(
            ogreMovableName(), meshname);
      } catch (Ogre::InvalidParametersException &) {
        SILOG(ogre,error,"Got invalid parameters");
        throw;
      }
    } catch (...) {
        SILOG(ogre,error,"Failed to load mesh "<< meshname << " (id "<<id()<<")!");
        return;
    }

    SILOG(ogre,detailed,"Bounding box: " << new_entity->getBoundingBox());

    unsigned int num_subentities=new_entity->getNumSubEntities();
    SHA256 random_seed=SHA256::computeDigest(id());
    const SHA256::Digest &digest=random_seed.rawData();
    Ogre::Vector4 parallax_steps(getScene()->parallaxSteps(), getScene()->parallaxShadowSteps(),0.0,1.0);
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
    fixTextures();

    mMeshLoaded = true;

    if (!mInitialAnimationName.empty()) {
      setAnimation(mInitialAnimationName);
      mInitialAnimationName = "";
    }
}

void Entity::loadBillboard(Mesh::BillboardPtr bboard, const String& meshname)
{
    // With the material in place, create the Billboard(Set)
    Ogre::BillboardSet* new_bbs = NULL;
    try {
      try {
          new_bbs = getScene()->getSceneManager()->createBillboardSet(ogreMovableName(), 1);
          std::string matname = ogreMaterialName(meshname, 0);
          new_bbs->setMaterialName(matname);
          if (bboard->facing == Mesh::Billboard::FACING_FIXED)
              new_bbs->setBillboardType(Ogre::BBT_PERPENDICULAR_COMMON);
          else if (bboard->facing == Mesh::Billboard::FACING_CAMERA)
              new_bbs->setBillboardType(Ogre::BBT_POINT);
          new_bbs->setDefaultWidth( (bboard->aspectRatio > 0.f) ? bboard->aspectRatio : 1.f);
          new_bbs->setDefaultHeight(1.f);
          new_bbs->createBillboard(Ogre::Vector3(0,0,0), Ogre::ColourValue(1.f, 1.f, 1.f, 1.f));
          // The BillboardSet ends up with some bizarre settings for
          // bounds. Force them to be what we want manually: radius =
          // 1 and bounds for the box that fit in there (1/sqrt(3) sides)
          new_bbs->setBounds(Ogre::AxisAlignedBox(Ogre::Vector3(-.57735f, -.57735f, -.57735f), Ogre::Vector3(.57735f, .57735f, .57735f)), 1.f);
      } catch (Ogre::InvalidParametersException &) {
        SILOG(ogre,error,"Got invalid parameters");
        throw;
      }
    } catch (...) {
        SILOG(ogre,error,"Failed to load billboard "<< meshname << " (id "<<id()<<")!");
        return;
    }

    SILOG(ogre,detailed,"Bounding box: " << new_bbs->getBoundingBox());

    init(new_bbs);
    fixTextures();
}

void Entity::unloadEntity() {
    if (getOgreEntity()) unloadMesh();
    else if (getOgreBillboard()) unloadBillboard();
}

void Entity::unloadMesh() {
    Ogre::Entity* meshObj = getOgreEntity();
    init(NULL);
    if (meshObj) {
        getScene()->getSceneManager()->destroyEntity(meshObj);
        mOgreObject=NULL;
    }
    // Even though these are common in both, we only want to do them if we had
    // an entity.
    mReplacedMaterials.clear();
    for(WebMaterialList::iterator it = mWebMaterials.begin(); it != mWebMaterials.end(); it++)
        WebViewManager::getSingleton().destroyWebView(*it);
    mWebMaterials.clear();

    mMeshLoaded = false;
}

void Entity::unloadBillboard() {
    Ogre::BillboardSet* bbObj = getOgreBillboard();
    init(NULL);
    if (bbObj) {
        getScene()->getSceneManager()->destroyBillboardSet(bbObj);
        mOgreObject = NULL;
    }
    // Even though these are common in both, we only want to do them if we had
    // an entity.
    mReplacedMaterials.clear();
    for(WebMaterialList::iterator it = mWebMaterials.begin(); it != mWebMaterials.end(); it++)
        WebViewManager::getSingleton().destroyWebView(*it);
    mWebMaterials.clear();
}

void Entity::setSelected(bool selected) {
      mSceneNode->showBoundingBox(selected);
}

void Entity::undisplay() {
    unloadEntity();
}

void Entity::display(Transfer::URI const& meshFile)
{
    if (meshFile.empty()) {
        unloadEntity();
        return;
    }

    // If it's the same mesh *and* we still have it around or are working on it, just leave it alone
    if (mURI == meshFile && (mAssetDownload || mOgreObject))
        return;

    mMeshLoaded = false;

    // Otherwise, start the download process
    mURI = meshFile;
    mURIString = meshFile.toString();

    SILOG(ogre,detailed,"Loading " << mURIString << "...");
    mAssetDownload =
        AssetDownloadTask::construct(
            mURI, getScene(), this->priority(),
            mScene->context()->mainStrand->wrap(
                std::tr1::bind(&Entity::createMesh, this, livenessToken())
            ));
}

bool Entity::tryInstantiateExistingMesh(const String& meshname) {
    Ogre::MeshPtr mp = Ogre::MeshManager::getSingleton().getByName(meshname);
    if (!mp.isNull()) {
        loadMesh(meshname);
        return true;
    }
    return false;
}


void Entity::createMesh(Liveness::Token alive) {
    if (!alive) return;

    // first reset any animations.
    setAnimation("");

    setDynamic(isMobile());
    if (mCurrentAnimation) {
      mCurrentAnimation->setEnabled(false);
      mCurrentAnimation = NULL;
    }
    mAnimationList.clear();

    //get the mesh data and check that it is valid.
    bool usingDefault = false;
    VisualPtr visptr = mAssetDownload->asset();

    AssetDownloadTaskPtr assetDownload(mAssetDownload);
    mAssetDownload=AssetDownloadTaskPtr();

    if (!visptr) {
        usingDefault = true;
        visptr = mScene->defaultMesh();
        if (!visptr) {
            notify(&EntityListener::entityLoaded, this, false);
            return;
        }
    }

    // Clear out any old data if we have any left
    unloadEntity();

    MeshdataPtr mdptr( std::tr1::dynamic_pointer_cast<Meshdata>(visptr) );
    if (mdptr) {
        createMeshdata(mdptr, usingDefault, assetDownload);
        notify(&EntityListener::entityLoaded, this, true);
        return;
    }

    BillboardPtr bbptr( std::tr1::dynamic_pointer_cast<Billboard>(visptr) );
    if (bbptr) {
        createBillboard(bbptr, usingDefault, assetDownload);
        notify(&EntityListener::entityLoaded, this, true);
        return;
    }

    // If we got here, we don't know how to load it
    SILOG(ogre, error, "Entity::createMesh failed because it doesn't know how to load " << visptr->type());
    notify(&EntityListener::entityLoaded, this, false);
}

SHA256 Entity::computeVisualHash(const Mesh::VisualPtr& visptr, AssetDownloadTaskPtr assetDownload) {
    // To compute a hash of the entire visual, we just combine the hashes of the
    // original and all the dependent resources, then hash the result. The
    // approach right now uses strings and could do somethiing like xor'ing
    // everything instead, but this is easy and shouldn't happen all that often.
    String data = visptr->hash.toString();

    for(AssetDownloadTask::Dependencies::const_iterator tex_it = assetDownload->dependencies().begin(); tex_it != assetDownload->dependencies().end(); tex_it++) {
        const AssetDownloadTask::ResourceData& tex_data = tex_it->second;
        data += tex_data.request->getMetadata().getFingerprint().toString();
    }

    return SHA256::computeDigest(data);
}

void Entity::loadDependentTextures(AssetDownloadTaskPtr assetDownload, bool usingDefault) {
    if (!usingDefault) { // we currently assume no dependencies for default
        for(AssetDownloadTask::Dependencies::const_iterator tex_it = assetDownload->dependencies().begin(); tex_it != assetDownload->dependencies().end(); tex_it++) {
            const AssetDownloadTask::ResourceData& tex_data = tex_it->second;
            if (mActiveCDNArchive && mTextureFingerprints->find(tex_data.request->getURI().toString()) == mTextureFingerprints->end() ) {
                String id = tex_data.request->getURI().toString() + tex_data.request->getMetadata().getFingerprint().toString();
                fixOgreURI(id);

                (*mTextureFingerprints)[tex_data.request->getURI().toString()] = id;

                // This could be a regular texture or a . If its ever a static
                // image, we want to decode it directly...
                FIMEMORY* mem_img_data = FreeImage_OpenMemory((BYTE*)tex_data.response->data(), tex_data.response->size());
                FREE_IMAGE_FORMAT fif = FreeImage_GetFileTypeFromMemory(mem_img_data);
                FreeImage_CloseMemory(mem_img_data);
                if (fif != FIF_UNKNOWN) {
                    CDNArchiveFactory::getSingleton().addArchiveData(mCDNArchive,id,SparseData(tex_data.response));
                }
                else if (tex_data.request->getURI().scheme() == "http") {
                    // Or, if its an http URL, we can try displaying it in a webview
                    OGRE_LOG(detailed,"Using webview for " << id << ": " << tex_data.request->getURI());
                    WebView* web_mat = WebViewManager::getSingleton().createWebViewMaterial(
                        mScene->context(),
                        id,
                        512, 512 // Completely arbitrary...
                    );
                    web_mat->loadURL(tex_data.request->getURI().toString());
                    mWebMaterials.push_back(web_mat);
                }
            }
        }
    }
}

void Entity::createMeshdata(const MeshdataPtr& mdptr, bool usingDefault, AssetDownloadTaskPtr assetDownload) {
    //Extract any animations from the new mesh.
    for (uint32 i=0;  i < mdptr->nodes.size(); i++) {
      Sirikata::Mesh::Node& node = mdptr->nodes[i];

      for (std::map<String, Sirikata::Mesh::TransformationKeyFrames>::iterator it = node.animations.begin();
           it != node.animations.end(); it++)
        {
          mAnimationList.insert(it->first);
        }
    }


    SHA256 sha = mdptr->hash;
    String hash = sha.convertToHexString();
    String meshname = computeVisualHash(mdptr, assetDownload).toString();

    // If we already have it, just load the existing one
    if (tryInstantiateExistingMesh(meshname)) return;

    loadDependentTextures(assetDownload, usingDefault);

    if (!mdptr->instances.empty()) {
        Ogre::MaterialManager& matm = Ogre::MaterialManager::getSingleton();
        int index=0;
        for (MaterialEffectInfoList::const_iterator mat=mdptr->materials.begin(),mate=mdptr->materials.end();mat!=mate;++mat,++index) {
            std::string matname = ogreMaterialName(meshname, index);
            Ogre::MaterialPtr matPtr=matm.getByName(matname);
            if (matPtr.isNull()) {
                Ogre::ManualResourceLoader * reload;
                matPtr = matm.create(
                    matname, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, true,
                    (reload=new ManualMaterialLoader (mdptr, matname, *mat, mURI, mTextureFingerprints))
                );

                reload->prepareResource(&*matPtr);
                reload->loadResource(&*matPtr);
            }
        }

        // Skeleton
        Ogre::SkeletonPtr skel(NULL);
        if (!mdptr->joints.empty()) {

            Ogre::SkeletonManager& skel_mgr = Ogre::SkeletonManager::getSingleton();
            Ogre::ManualResourceLoader *reload;
            skel = Ogre::SkeletonPtr(skel_mgr.create(ogreSkeletonName(meshname),Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, true,
                                                     (reload=new ManualSkeletonLoader(mdptr, mAnimationList))));
            reload->prepareResource(&*skel);
            reload->loadResource(&*skel);

            if (! ((ManualSkeletonLoader*)reload)->wasSkeletonLoaded()) {
              skel.setNull();
            }
        }


        // Mesh
        {
            Ogre::MeshManager& mm = Ogre::MeshManager::getSingleton();
            /// FIXME: set bounds, bounding radius here
            Ogre::ManualResourceLoader *reload;
            Ogre::MeshPtr mo (mm.createManual(meshname,Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,(reload=
#ifdef _WIN32
#ifdef NDEBUG
			OGRE_NEW
#else
			new
#endif
#else
			OGRE_NEW
#endif
                        ManualMeshLoader(mdptr, meshname))));
            reload->prepareResource(&*mo);
            reload->loadResource(&*mo);

            if (!skel.isNull())
                mo->_notifySkeleton(skel);


            bool check = mm.resourceExists(meshname);
        }

        loadMesh(meshname);
    }

    // Lights
    int light_idx = 0;
    Meshdata::LightInstanceIterator lightinst_it = mdptr->getLightInstanceIterator();
    uint32 lightinst_idx;
    Matrix4x4f pos_xform;
    while( lightinst_it.next(&lightinst_idx, &pos_xform) ) {
        const LightInstance& lightinst = mdptr->lightInstances[lightinst_idx];

        // Get the instanced submesh
        if(lightinst.lightIndex >= (int)mdptr->lights.size()){
            SILOG(ogre,error, "bad light index %d for lights only sized %d\n"<<lightinst.lightIndex<<"/"<<(int)mdptr->lights.size());
            continue;
        }
        const LightInfo& sublight = mdptr->lights[lightinst.lightIndex];

        String lightname = ogreLightName(mName, meshname, light_idx++);
        Ogre::Light* light = constructOgreLight(getScene()->getSceneManager(), lightname, sublight);
        if (!light->isAttached()) {
            mLights.push_back(light);

            // Lights just assume local position at the origin. We just need to
            // transform appropriately.
            Vector4f v_xform = pos_xform * Vector4f(0.f, 0.f, 0.f, 1.f);
            // The light has an extra scene node to handle the specific transformation
            Ogre::SceneNode* xformnode = mScene->getSceneManager()->createSceneNode();
            xformnode->translate(v_xform[0], v_xform[1], v_xform[2]);
            // Rotation needs to b handled by extracting rotation information.
            Quaternion qrot(pos_xform.extract3x3());
            xformnode->rotate(toOgre(qrot));
            xformnode->attachObject(light);

            mSceneNode->addChild(xformnode);
            //light->setDebugDisplayEnabled(true);
        }
    }
}

void Entity::createBillboard(const BillboardPtr& bbptr, bool usingDefault, AssetDownloadTaskPtr assetDownload) {
    SHA256 sha = bbptr->hash;
    String hash = sha.convertToHexString();
    String bbname = computeVisualHash(bbptr, assetDownload).toString();

    loadDependentTextures(assetDownload, usingDefault);

    // Load the single material
    Ogre::MaterialManager& matm = Ogre::MaterialManager::getSingleton();

    std::string matname = ogreMaterialName(bbname, 0);
    Ogre::MaterialPtr matPtr = matm.getByName(matname);
    if (matPtr.isNull()) {
        Ogre::ManualResourceLoader* reload;

        // We need to fill in a MaterialEffectInfo because the loader we're
        // using only knows how to process them for Meshdatas right now
        MaterialEffectInfo matinfo;
        matinfo.shininess = 0.0f;
        matinfo.reflectivity = 1.0f;
        matinfo.textures.push_back(MaterialEffectInfo::Texture());
        MaterialEffectInfo::Texture& tex = matinfo.textures.back();
        tex.uri = bbptr->image;
        tex.color = Vector4f(1.f, 1.f, 1.f, 1.f);
        tex.texCoord = 0;
        tex.affecting = MaterialEffectInfo::Texture::AMBIENT;
        tex.samplerType = MaterialEffectInfo::Texture::SAMPLER_TYPE_2D;
        tex.minFilter = MaterialEffectInfo::Texture::SAMPLER_FILTER_LINEAR_MIPMAP_LINEAR;
        tex.magFilter = MaterialEffectInfo::Texture::SAMPLER_FILTER_LINEAR_MIPMAP_LINEAR;
        tex.wrapS = MaterialEffectInfo::Texture::WRAP_MODE_NONE;
        tex.wrapT = MaterialEffectInfo::Texture::WRAP_MODE_NONE;
        tex.wrapU = MaterialEffectInfo::Texture::WRAP_MODE_NONE;
        tex.mipBias = 0.0f;
        tex.maxMipLevel = 20;

        matPtr = matm.create(
            matname, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, true,
            (reload = new ManualMaterialLoader (bbptr, matname, matinfo, mURI, mTextureFingerprints))
        );

        reload->prepareResource(&*matPtr);
        reload->loadResource(&*matPtr);
    }

    // The BillboardSet that actually gets rendered is like an Ogre::Entity,
    // load it in a similar way. There is no equivalent of the Ogre::Mesh -- we
    // only need to load up the material
    loadBillboard(bbptr, bbname);
}

const std::vector<String> Entity::getAnimationList() {
  std::vector<String> animations;
  for (std::set<String>::iterator it = mAnimationList.begin(); it != mAnimationList.end(); it++) {
    animations.push_back(*it);
  }

  return animations;
}

}
}

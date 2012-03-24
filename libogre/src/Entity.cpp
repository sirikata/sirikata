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
#include <sirikata/ogre/OgreHeaders.hpp>
#include <sirikata/ogre/Lights.hpp>
#include <sirikata/core/transfer/URL.hpp>
#include <sirikata/ogre/Util.hpp>
#include <sirikata/mesh/Bounds.hpp>
#include <OgreEntity.h>
#include <OgreSubEntity.h>
#include <OgreBillboardSet.h>
#include <OgreAnimationState.h>
#include <OgreMeshManager.h>

using namespace Sirikata::Transfer;

namespace Sirikata {
namespace Graphics {

using namespace Sirikata::Mesh;


EntityListener::~EntityListener() {
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
   mIsAggregate(false),
   mAggregateRadius(0),
   mAggregateOffset(Vector3d(0,0,0)),
   mCurrentAnimation(NULL),
   mInitialAnimationName(""),
   mMeshName(""),
   mMeshLoaded(false)
{
    mSceneNode->setInheritScale(false);
    addToScene(NULL);

    bool successful = scene->mSceneEntities.insert(
        OgreRenderer::SceneEntitiesMap::value_type(mName, this)).second;

    assert (successful == true);

    unload();
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

    bool mobVal = false;
    removeFromScene(&mobVal);
    init(NULL);
    mSceneNode->detachAllObjects();
    /* detaches all children from the scene.
       There should be none, as the server should have adjusted their parents.
     */
    mSceneNode->removeAllChildren();
    mScene->getSceneManager()->destroySceneNode(mSceneNode);
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

Ogre::Entity* Entity::getOgreEntity()  {
  return dynamic_cast<Ogre::Entity*> (mOgreObject);
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
        updateScale( getRadius() );
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

void Entity::setIsAggregate(bool isAgg) {
  mIsAggregate = isAgg;
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

void Entity::setAnimation(const String& name, bool* mobileVal)
{
    // Disable current animation if we have one
    if (mCurrentAnimation) {
        mCurrentAnimation->setEnabled(false);
        mCurrentAnimation = NULL;
        if (mobileVal)
            setDynamic(mobileVal);
        else
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

void Entity::removeFromScene(bool* checkMobile)
{
    Ogre::SceneNode *oldParent = mSceneNode->getParentSceneNode();
    if (oldParent) {
        oldParent->removeChild(mSceneNode);
    }
    // Force dynamicity off
    setAnimation("",checkMobile);
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
    Ogre::Vector3 ogrepos = toOgre(pos+mAggregateOffset, getScene()->getOffset());
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

float Entity::getRadius() {
  if (mAggregateRadius != 0)
    return mAggregateRadius;

  return this->bounds().radius();
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

namespace {

// If a mesh had already been downloaded (by us or someone else), we have
// the hash, *and* the object still exists, we should be able to just add it
// to the scene.
bool ogreHasAsset(const String& meshname) {
    Ogre::MeshPtr mp = Ogre::MeshManager::getSingleton().getByName(meshname);
    return !mp.isNull();
}

}

void Entity::beginLoad() {
    // first reset any animations.
    setAnimation("");

    setDynamic(isMobile());
    if (mCurrentAnimation) {
      mCurrentAnimation->setEnabled(false);
      mCurrentAnimation = NULL;
    }
    mAnimationList.clear();

    // Clear out any old data if we have any left
    unload();
}

void Entity::loadEmpty() {
    beginLoad();
    mMeshLoaded = false;
}

void Entity::loadMesh(Mesh::MeshdataPtr meshdata, const String& meshname, const std::set<String>& animations)
{
    beginLoad();
    mMeshName = meshdata->uri;    

    mAnimationList = animations;

    if (!ogreHasAsset(meshname)) {
        SILOG(ogre, error, "Requested load of asset that hasn't been loaded into Ogre (" << meshname << ")");
        return;
    }

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

    //Special treatment for aggregates, so that they overlap with the individual
    //objects they are composed. This special treatment is ok, since aggregates are 
    //generated by space server, not by some federated third party.
    if (mIsAggregate) {      
      BoundingBox3f3f originalMeshBoundingBox = BoundingBox3f3f::null();
      double originalMeshBoundsRadius=0;
      ComputeBounds( meshdata, &originalMeshBoundingBox, &originalMeshBoundsRadius);

      mAggregateRadius = originalMeshBoundsRadius;
      updateScale(originalMeshBoundsRadius);      

      Vector4f offsetFromCenter = meshdata->globalTransform.getCol(3);
      offsetFromCenter = offsetFromCenter * -1.f;      
      Vector3d aggregateOffset = Vector3d(offsetFromCenter.x, offsetFromCenter.y, offsetFromCenter.z);
      
      setOgrePosition(getOgrePosition() + aggregateOffset);
      mAggregateOffset = aggregateOffset;
    }
    
    
    fixTextures();

    mMeshLoaded = true;

    if (!mInitialAnimationName.empty()) {
      setAnimation(mInitialAnimationName);
      mInitialAnimationName = "";
    }

    notify(&EntityListener::entityLoaded, this, true);
}

void Entity::loadBillboard(Mesh::BillboardPtr bboard, const String& bbtexname)
{
    beginLoad();

    // With the material in place, create the Billboard(Set)
    Ogre::BillboardSet* new_bbs = NULL;
    try {
      try {
          new_bbs = getScene()->getSceneManager()->createBillboardSet(ogreMovableName(), 1);
          std::string matname = bbtexname;
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
        SILOG(ogre,error,"Failed to load billboard "<< bbtexname << " (id "<<id()<<")!");
        return;
    }

    SILOG(ogre,detailed,"Bounding box: " << new_bbs->getBoundingBox());

    init(new_bbs);
    fixTextures();

    notify(&EntityListener::entityLoaded, this, true);
}

void Entity::loadFailed() {
    notify(&EntityListener::entityLoaded, this, false);
}

void Entity::unload() {  
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
}

void Entity::setSelected(bool selected) {
      mSceneNode->showBoundingBox(selected);
      SILOG(ogre,detailed,  "Selected mesh: " << mMeshName << " with radius: " << getRadius()  << " at position " <<  getOgrePosition() );
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

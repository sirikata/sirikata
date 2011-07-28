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
	const Entity::TextureBindingsMap &mFrom;
public:
	ShouldReplaceTexture(const Entity::TextureBindingsMap &from)
		: mFrom(from) {
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
   mInitialAnimationName("")
{
    mTextureFingerprints = std::tr1::shared_ptr<TextureBindingsMap>(new TextureBindingsMap());

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

    if (mOgreObject == NULL) {
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
    unloadEntity();

    mReplacedMaterials.clear();

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
}

void Entity::loadBillboard(Mesh::BillboardPtr bboard, const String& meshname)
{
    unloadEntity();
    mReplacedMaterials.clear();

    // With the material in place, create the Billboard(Set)
    Ogre::BillboardSet* new_bbs = NULL;
    try {
      try {
          new_bbs = getScene()->getSceneManager()->createBillboardSet(ogreMovableName(), 1);
          std::string matname = meshname + "_mat_billboard_";
          new_bbs->setMaterialName(matname);
          if (bboard->facing == Mesh::Billboard::FACING_FIXED)
              new_bbs->setBillboardType(Ogre::BBT_PERPENDICULAR_COMMON);
          else if (bboard->facing == Mesh::Billboard::FACING_CAMERA)
              new_bbs->setBillboardType(Ogre::BBT_POINT);
          new_bbs->setDefaultWidth( (bboard->aspectRatio > 0.f) ? bboard->aspectRatio : 1.f);
          new_bbs->setDefaultHeight(1.f);
          new_bbs->createBillboard(Ogre::Vector3(0,0,0), Ogre::ColourValue(1.f, 1.f, 1.f, 1.f));
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
    mReplacedMaterials.clear();
}

void Entity::unloadBillboard() {
    Ogre::BillboardSet* bbObj = getOgreBillboard();
    init(NULL);
    if (bbObj) {
        getScene()->getSceneManager()->destroyBillboardSet(bbObj);
        mOgreObject = NULL;
    }
    mReplacedMaterials.clear();
}

void Entity::setSelected(bool selected) {
      mSceneNode->showBoundingBox(selected);
}


void Entity::processMesh(Transfer::URI const& meshFile)
{
    if (meshFile.empty()) {
        unloadEntity();
        return;
    }

    // If it's the same mesh *and* we still have it around or are working on it, just leave it alone
    if (mURI == meshFile && (mAssetDownload || mOgreObject))
        return;

    // Otherwise, start the download process
    mURI = meshFile;
    mURIString = meshFile.toString();

    mAssetDownload =
        AssetDownloadTask::construct(
            mURI, getScene(), this->priority(),
            mScene->context()->mainStrand->wrap(
                std::tr1::bind(&Entity::createMesh, this, livenessToken())
            ));
}

Ogre::TextureUnitState::TextureAddressingMode translateWrapMode(MaterialEffectInfo::Texture::WrapMode w) {
    switch(w) {
      case MaterialEffectInfo::Texture::WRAP_MODE_CLAMP:
        SILOG(ogre,insane,"CLAMPING");
        return Ogre::TextureUnitState::TAM_CLAMP;
      case MaterialEffectInfo::Texture::WRAP_MODE_MIRROR:
        SILOG(ogre,insane,"MIRRORING");
        return Ogre::TextureUnitState::TAM_MIRROR;
      case MaterialEffectInfo::Texture::WRAP_MODE_WRAP:
      default:
        SILOG(ogre,insane,"WRAPPING");
        return Ogre::TextureUnitState::TAM_WRAP;
    }
}

void fixupTextureUnitState(Ogre::TextureUnitState*tus, const MaterialEffectInfo::Texture&tex) {
    tus->setTextureAddressingMode(translateWrapMode(tex.wrapS),
                                  translateWrapMode(tex.wrapT),
                                  translateWrapMode(tex.wrapU));

}
class MaterialManualLoader : public Ogre::ManualResourceLoader {
    VisualPtr mVisualPtr;
    const MaterialEffectInfo* mMat;
    String mName;
    Transfer::URI mURI;
    std::tr1::shared_ptr<Entity::TextureBindingsMap> mTextureFingerprints;
public:
    MaterialManualLoader(
        VisualPtr visptr,
        const String name,
        const MaterialEffectInfo&mat,
        const Transfer::URI& uri,
        std::tr1::shared_ptr<Entity::TextureBindingsMap> textureFingerprints)
     : mTextureFingerprints(textureFingerprints)
    {
        mVisualPtr = visptr;
        mName = name;
        mMat=&mat;
        mURI=uri;
    }
    void prepareResource(Ogre::Resource*r){}
    void loadResource(Ogre::Resource *r) {
        using namespace Ogre;
        Material* material= dynamic_cast <Material*> (r);
        material->setCullingMode(CULL_NONE);
        Ogre::Technique* tech=material->getTechnique(0);
        bool useAlpha=false;
        if (mMat->textures.empty()) {
            Ogre::Pass*pass=tech->getPass(0);
            pass->setDiffuse(ColourValue(1,1,1,1));
            pass->setAmbient(ColourValue(1,1,1,1));
            pass->setSelfIllumination(ColourValue(0,0,0,0));
            pass->setSpecular(ColourValue(1,1,1,1));
        }
        for (size_t i=0;i<mMat->textures.size();++i) {
            // NOTE: We're currently disabling using alpha when the
            // texture is specified for OPACITY because we're not
            // handling it in the code below, leading to artifacts as
            // it seemingly uses random segments of memory for the
            // alpha values (maybe because it tries reading them from
            // some texture other texture that doesn't specify
            // alpha?).
            if (mMat->textures[i].affecting==MaterialEffectInfo::Texture::OPACITY &&
                (/*mMat->textures[i].uri.length() > 0 ||*/
                    (mMat->textures[i].uri.length() == 0 && mMat->textures[i].color.w<1.0))) {
                useAlpha=true;
                break;
            }
        }
        unsigned int valid_passes=0;
        {
            Ogre::Pass* pass = tech->getPass(0);
            pass->setAmbient(ColourValue(0,0,0,0));
            pass->setDiffuse(ColourValue(0,0,0,0));
            pass->setSelfIllumination(ColourValue(0,0,0,0));
            pass->setSpecular(ColourValue(0,0,0,0));
        }
        for (size_t i=0;i<mMat->textures.size();++i) {
            MaterialEffectInfo::Texture tex=mMat->textures[i];
            Ogre::Pass*pass=tech->getPass(0);
            if (tex.uri.length()==0) { // Flat colors
                switch (tex.affecting) {
                case MaterialEffectInfo::Texture::DIFFUSE:
                  pass->setDiffuse(ColourValue(tex.color.x,
                                               tex.color.y,
                                               tex.color.z,
                                               tex.color.w));
                  break;
                case MaterialEffectInfo::Texture::AMBIENT:
                  pass->setAmbient(ColourValue(tex.color.x,
                                               tex.color.y,
                                               tex.color.z,
                                               tex.color.w));
                  break;
                case MaterialEffectInfo::Texture::EMISSION:
                  pass->setSelfIllumination(ColourValue(tex.color.x,
                                                        tex.color.y,
                                                        tex.color.z,
                                                        tex.color.w));
                  break;
                case MaterialEffectInfo::Texture::SPECULAR:
                  pass->setSpecular(ColourValue(tex.color.x,
                                                tex.color.y,
                                                tex.color.z,
                                                tex.color.w));
                  pass->setShininess(mMat->shininess);
                  break;
                default:
                  break;
                }
            } else if (tex.affecting==MaterialEffectInfo::Texture::DIFFUSE) { // or textured
                // FIXME other URI schemes besides URL
                Transfer::URL url(mURI);
                assert(!url.empty());
                Transfer::URI mat_uri( URL(url.context(), tex.uri).toString() );
                Entity::TextureBindingsMap::iterator where = mTextureFingerprints->find(mat_uri.toString());
                if (where!=mTextureFingerprints->end()) {
                    String ogreTextureName = where->second;
                    fixOgreURI(ogreTextureName);
                    Ogre::TextureUnitState*tus;
                    //tus->setTextureName(tex.uri);
                    //tus->setTextureCoordSet(tex.texCoord);
                    if (useAlpha==false) {
                        pass->setAlphaRejectValue(.5);
                        pass->setAlphaRejectSettings(CMPF_GREATER,128,true);
                        if (true||i==0) {
                            pass->setSceneBlending(SBF_ONE,SBF_ZERO);
                        } else {
                            pass->setSceneBlending(SBF_ONE,SBF_ONE);
                        }
                    }else {
                        pass->setDepthWriteEnabled(false);
                        pass->setDepthCheckEnabled(true);
                        if (true||i==0) {
                            pass->setSceneBlending(SBF_SOURCE_ALPHA,SBF_ONE_MINUS_SOURCE_ALPHA);
                        }else {
                            pass->setSceneBlending(SBF_SOURCE_ALPHA,SBF_ONE);
                        }
                    }
                    switch (tex.affecting) {
                      case MaterialEffectInfo::Texture::DIFFUSE:
                        if (tech->getNumPasses()<=valid_passes) {
                            pass=tech->createPass();
                            ++valid_passes;
                        }
                        pass->setDiffuse(ColourValue(1,1,1,1));
/*
                        pass->setAmbient(ColourValue(0,0,0,0));
                        pass->setSelfIllumination(ColourValue(0,0,0,0));
                        pass->setSpecular(ColourValue(0,0,0,0));
*/
                        //pass->setIlluminationStage(IS_PER_LIGHT);
                        if (pass->getTextureUnitState(ogreTextureName) == 0) {
                          tus=pass->createTextureUnitState(ogreTextureName,tex.texCoord);
                          fixupTextureUnitState(tus,tex);
                          tus->setColourOperation(LBO_MODULATE);
                        }

                        break;
                      case MaterialEffectInfo::Texture::AMBIENT:
                        if (tech->getNumPasses()<=valid_passes) {
                            pass=tech->createPass();
                            ++valid_passes;
                        }

                        pass->setDiffuse(ColourValue(0,0,0,0));
                        pass->setSelfIllumination(ColourValue(0,0,0,0));
                        pass->setSpecular(ColourValue(0,0,0,0));

                        pass->setAmbient(ColourValue(1,1,1,1));
                        //tus->setColourOperation(LBO_MODULATE);

                        pass->setIlluminationStage(IS_AMBIENT);
                        break;
                      case MaterialEffectInfo::Texture::EMISSION:
/*
                        pass->setDiffuse(ColourValue(0,0,0,0));
                        pass->setAmbient(ColourValue(0,0,0,0));
                        pass->setSpecular(ColourValue(0,0,0,0));
*/
                        if (pass->getTextureUnitState(ogreTextureName) == 0) {
                          tus=pass->createTextureUnitState(ogreTextureName,tex.texCoord);
                          fixupTextureUnitState(tus,tex);
                          //pass->setSelfIllumination(ColourValue(1,1,1,1));
                          tus->setColourOperation(LBO_ADD);
                        }

                        //pass->setIlluminationStage(IS_DECAL);
                        break;
                      case MaterialEffectInfo::Texture::SPECULAR:
                        if (tech->getNumPasses()<=valid_passes) {
                            pass=tech->createPass();
                            ++valid_passes;
                        }

                        pass->setDiffuse(ColourValue(0,0,0,0));
                        pass->setAmbient(ColourValue(0,0,0,0));
                        pass->setSelfIllumination(ColourValue(0,0,0,0));
                        //pass->setIlluminationStage(IS_PER_LIGHT);

                        if (pass->getTextureUnitState(ogreTextureName) == 0) {
                          tus=pass->createTextureUnitState(ogreTextureName,tex.texCoord);
                          fixupTextureUnitState(tus,tex);
                          tus->setColourOperation(LBO_MODULATE);
                        }
                        pass->setSpecular(ColourValue(1,1,1,1));
                        break;
                      default:
                  break;
                    }
                }
            }
        }
    }

};

class SkeletonManualLoader : public Ogre::ManualResourceLoader {
public:
  SkeletonManualLoader(MeshdataPtr meshdata, const std::set<String>& animationList)
    : mdptr(meshdata), skeletonLoaded(false), animations(animationList)
    {
    }

    void prepareResource(Ogre::Resource*r) {
    }

  double invert(Matrix4x4f& inv, const Matrix4x4f& orig)
  {
    float mat[16];
    float dst[16];

    int counter = 0;
    for (int i=0; i<4; i++) {
      for (int j=0; j<4; j++) {
        mat[counter] = orig(i,j);
        counter++;
      }
    }

    float tmp[12]; /* temp array for pairs */
    float src[16]; /* array of transpose source matrix */
    float det; /* determinant */
    /* transpose matrix */
    for (int i = 0; i < 4; i++) {
      src[i] = mat[i*4];
      src[i + 4] = mat[i*4 + 1];
      src[i + 8] = mat[i*4 + 2];
      src[i + 12] = mat[i*4 + 3];
    }
    /* calculate pairs for first 8 elements (cofactors) */
    tmp[0] = src[10] * src[15];
    tmp[1] = src[11] * src[14];
    tmp[2] = src[9] * src[15];
    tmp[3] = src[11] * src[13];
    tmp[4] = src[9] * src[14];
    tmp[5] = src[10] * src[13];
    tmp[6] = src[8] * src[15];
    tmp[7] = src[11] * src[12];
    tmp[8] = src[8] * src[14];
    tmp[9] = src[10] * src[12];
    tmp[10] = src[8] * src[13];
    tmp[11] = src[9] * src[12];
    /* calculate first 8 elements (cofactors) */
    dst[0] = tmp[0]*src[5] + tmp[3]*src[6] + tmp[4]*src[7];
    dst[0] -= tmp[1]*src[5] + tmp[2]*src[6] + tmp[5]*src[7];
    dst[1] = tmp[1]*src[4] + tmp[6]*src[6] + tmp[9]*src[7];
    dst[1] -= tmp[0]*src[4] + tmp[7]*src[6] + tmp[8]*src[7];
    dst[2] = tmp[2]*src[4] + tmp[7]*src[5] + tmp[10]*src[7];
    dst[2] -= tmp[3]*src[4] + tmp[6]*src[5] + tmp[11]*src[7];
    dst[3] = tmp[5]*src[4] + tmp[8]*src[5] + tmp[11]*src[6];
    dst[3] -= tmp[4]*src[4] + tmp[9]*src[5] + tmp[10]*src[6];
    dst[4] = tmp[1]*src[1] + tmp[2]*src[2] + tmp[5]*src[3];
    dst[4] -= tmp[0]*src[1] + tmp[3]*src[2] + tmp[4]*src[3];
    dst[5] = tmp[0]*src[0] + tmp[7]*src[2] + tmp[8]*src[3];
    dst[5] -= tmp[1]*src[0] + tmp[6]*src[2] + tmp[9]*src[3];
    dst[6] = tmp[3]*src[0] + tmp[6]*src[1] + tmp[11]*src[3];
    dst[6] -= tmp[2]*src[0] + tmp[7]*src[1] + tmp[10]*src[3];
    dst[7] = tmp[4]*src[0] + tmp[9]*src[1] + tmp[10]*src[2];
    dst[7] -= tmp[5]*src[0] + tmp[8]*src[1] + tmp[11]*src[2];
    /* calculate pairs for second 8 elements (cofactors) */
    tmp[0] = src[2]*src[7];
    tmp[1] = src[3]*src[6];
    tmp[2] = src[1]*src[7];
    tmp[3] = src[3]*src[5];
    tmp[4] = src[1]*src[6];
    tmp[5] = src[2]*src[5];
    tmp[6] = src[0]*src[7];
    tmp[7] = src[3]*src[4];
    tmp[8] = src[0]*src[6];
    tmp[9] = src[2]*src[4];
    tmp[10] = src[0]*src[5];
    tmp[11] = src[1]*src[4];
    /* calculate second 8 elements (cofactors) */
    dst[8] = tmp[0]*src[13] + tmp[3]*src[14] + tmp[4]*src[15];
    dst[8] -= tmp[1]*src[13] + tmp[2]*src[14] + tmp[5]*src[15];
    dst[9] = tmp[1]*src[12] + tmp[6]*src[14] + tmp[9]*src[15];
    dst[9] -= tmp[0]*src[12] + tmp[7]*src[14] + tmp[8]*src[15];
    dst[10] = tmp[2]*src[12] + tmp[7]*src[13] + tmp[10]*src[15];
    dst[10]-= tmp[3]*src[12] + tmp[6]*src[13] + tmp[11]*src[15];
    dst[11] = tmp[5]*src[12] + tmp[8]*src[13] + tmp[11]*src[14];
    dst[11]-= tmp[4]*src[12] + tmp[9]*src[13] + tmp[10]*src[14];
    dst[12] = tmp[2]*src[10] + tmp[5]*src[11] + tmp[1]*src[9];
    dst[12]-= tmp[4]*src[11] + tmp[0]*src[9] + tmp[3]*src[10];
    dst[13] = tmp[8]*src[11] + tmp[0]*src[8] + tmp[7]*src[10];
    dst[13]-= tmp[6]*src[10] + tmp[9]*src[11] + tmp[1]*src[8];
    dst[14] = tmp[6]*src[9] + tmp[11]*src[11] + tmp[3]*src[8];
    dst[14]-= tmp[10]*src[11] + tmp[2]*src[8] + tmp[7]*src[9];
    dst[15] = tmp[10]*src[10] + tmp[4]*src[8] + tmp[9]*src[9];
    dst[15]-= tmp[8]*src[9] + tmp[11]*src[10] + tmp[5]*src[8];
    /* calculate determinant */
    det=src[0]*dst[0]+src[1]*dst[1]+src[2]*dst[2]+src[3]*dst[3];

    if (det == 0.0) return 0.0;

    /* calculate matrix inverse */
    det = 1/det;
    for (int j = 0; j < 16; j++)
      dst[j] *= det;

    counter = 0;
    for (int i=0; i<4; i++) {
      for (int j=0; j<4; j++) {
        inv(i,j) = dst[counter];
        counter++;
      }
    }

    return det;
  }

  bool getTRS(const Matrix4x4f& bsm, Ogre::Vector3& translate, Ogre::Quaternion& quaternion, Ogre::Vector3& scale) {
    Vector4f trans = bsm.getCol(3);

    // Get the scaling matrix
    float32 scaleX = bsm.getCol(0).length();
    float32 scaleY = bsm.getCol(1).length();
    float32 scaleZ = bsm.getCol(2).length();

    // Get the rotation quaternion
    Vector4f xrot =  bsm.getCol(0)/scaleX;
    Vector4f yrot =  bsm.getCol(1)/scaleY;
    Vector4f zrot =  bsm.getCol(2)/scaleY;
    Matrix4x4f rotmat(xrot, yrot, zrot, Vector4f(0,0,0,1), Matrix4x4f::COLUMNS());

    float32 trace = rotmat(0,0) + rotmat(1,1) + rotmat(2,2) ;

    float32 qw,qx,qy,qz;
    if (trace > 0) {
      float32 S = sqrt(trace + 1);

      qw =  0.5f * S ;
      S = 0.5f / S;
      qx = (rotmat(2,1)-rotmat(1,2)) * S;
      qy = (rotmat(0,2)-rotmat(2,0)) * S ;
      qz = (rotmat(1,0)-rotmat(0,1)) * S;
    }
    else {
      //code in this block copied from Ogre Quaternion...

      static size_t s_iNext[3] = { 1, 2, 0 };
      size_t i = 0;
      if ( rotmat(1,1) > rotmat(0,0) )
        i = 1;
      if ( rotmat(2,2) > rotmat(i,i) )
        i = 2;
      size_t j = s_iNext[i];
      size_t k = s_iNext[j];

      float32 fRoot = sqrt(rotmat(i,i)-rotmat(j,j)-rotmat(k,k) + 1.0f);
      float32* apkQuat[3] = { &qx, &qy, &qz };
      *apkQuat[i] = 0.5f*fRoot;
      fRoot = 0.5f/fRoot;
      qw = (rotmat(k,j)-rotmat(j,k))*fRoot;
      *apkQuat[j] = (rotmat(j,i)+rotmat(i,j))*fRoot;
      *apkQuat[k] = (rotmat(k,i)+rotmat(i,k))*fRoot;
    }

    float32 N = sqrt(qx*qx + qy*qy + qz*qz + qw*qw);
    qx /= N; qy /= N; qz /= N; qw /= N;

    Matrix4x4f scalemat( Matrix4x4f::identity()  );
    scalemat(0,0) = scaleX;
    scalemat(1,1) = scaleY;
    scalemat(2,2) = scaleZ;

    Matrix4x4f transmat(Matrix4x4f::identity());
    transmat(0,3) = trans.x;
    transmat(1,3) = trans.y;
    transmat(2,3) = trans.z;

    Matrix4x4f diffmat = (bsm - (transmat*rotmat*scalemat));
    float32 matlen = diffmat.getCol(0).length() + diffmat.getCol(1).length() + diffmat.getCol(2).length()
      + diffmat.getCol(3).length() ;

    if (matlen > 0.00001) {
      return false;
    }

    quaternion = Ogre::Quaternion(qw,
                                  qx,
                                  qy, qz);

    translate = Ogre::Vector3(trans.x, trans.y, trans.z);

    scale = Ogre::Vector3( scaleX, scaleY, scaleZ);

    return true;
  }

  void loadResource(Ogre::Resource *r) {
        Ogre::Skeleton* skel = dynamic_cast<Ogre::Skeleton*> (r);

        // We have to set some binding information for each joint (bind shape
        // matrix * inverseBindMatrix_i for each joint i). Collada has a more
        // flexible animation system, so here we only support this if there is 1
        // SubMeshGeometry with 1 SkinController
        if (mdptr->geometry.size() != 1 || mdptr->geometry[0].skinControllers.size() != 1) {
            SILOG(ogre,error,"Only know how to handle skinning for 1 SubMeshGeometry and 1 SkinController. Leaving skeleton unbound.");
            return;
        }
        const SkinController& skin = mdptr->geometry[0].skinControllers[0];

        typedef std::map<uint32, Ogre::Bone*> BoneMap;
        BoneMap bones;

        Ogre::Vector3 translate(0,0,0), scale(1,1,1);
        Ogre::Quaternion rotate(1,0,0,0);
        Meshdata::JointIterator joint_it = mdptr->getJointIterator();
        uint32 joint_id;
        uint32 joint_idx;
        Matrix4x4f pos_xform;
        uint32 parent_id;
        std::vector<Matrix4x4f> transformList;

        // We build animations as we go since the Meshdata doesn't
        // have a central index of animations -- we need to add them
        // as we encounter them and then adjust them, adding tracks
        // for different bones, as we go.
        typedef std::map<String, Ogre::Animation*> AnimationMap;
        AnimationMap animationMap;

        // Bones
        // Basic root bone, no xform

        Matrix4x4f bsm = skin.bindShapeMatrix;
        Matrix4x4f B_inv;
        invert(B_inv, bsm);

        bones[0] = skel->createBone(0);

        unsigned short curBoneValue = mdptr->getJointCount()+1;

        std::tr1::unordered_map<uint32, Matrix4x4f> ibmMap;

        while( joint_it.next(&joint_id, &joint_idx, &pos_xform, &parent_id, transformList) ) {          

          // We need to work backwards from the joint_idx (index into
          // mdptr->joints) to the index of the joint in this skin controller
          // (so we can lookup inverseBindMatrices).
          uint32 skin_joint_idx = 0;
          for(skin_joint_idx = 0; skin_joint_idx < skin.joints.size(); skin_joint_idx++) {
            if (skin.joints[skin_joint_idx] == joint_idx) break;
          }            

          Matrix4x4f ibm = Matrix4x4f::identity();
          if (skin_joint_idx < skin.joints.size()) {
            // Get the bone's inverse bind-pose matrix and store it in the ibmMap.
            ibm = skin.inverseBindMatrices[skin_joint_idx];
          }
          
          ibmMap[joint_id] = ibm;

          /* Now construct the bone hierarchy. First implement the transform hierarchy from root to the bone's node. */
          Ogre::Bone* bone = bones[parent_id];

          if (bone == NULL) {
            SILOG(ogre, error, "Could not load skeleton for this mesh. This format is not currently supported");
            return;
          }

          if (parent_id == 0) {
            for (uint32 i = 0;   i < transformList.size() ; i++) {
              Matrix4x4f mat = transformList[i];

              bool ret = getTRS(mat, translate, rotate, scale);
              assert(ret);

              bone = bone->createChild(curBoneValue++, translate, rotate);
              bone->setScale(scale);
            }
          }

          const Node& node = mdptr->nodes[ mdptr->joints[joint_idx] ];          

          /* Finally create the actual bone */                              
          
          bone = bone->createChild(joint_id);          

          bones[joint_id] = bone;

          for (std::set<String>::const_iterator anim_it = animations.begin(); anim_it != animations.end(); anim_it++) {
            // Find/create the animation
            const String& anim_name = *anim_it;
            
            if (animationMap.find(anim_name) == animationMap.end())
              animationMap[anim_name] = skel->createAnimation(anim_name, 0.0);
            Ogre::Animation* anim = animationMap[anim_name];

            Ogre::Vector3 startPos(0,0,0);
            Ogre::Quaternion startOrientation(1,0,0,0);
            Ogre::Vector3 startScale(1,1,1);

            // If this node has associated animation keyframes with this name, then use those keyframes.
            if (node.animations.find(anim_name) != node.animations.end()) {
              const TransformationKeyFrames& anim_key_frames = node.animations.find(anim_name)->second;

              // Add track, making sure we set the length long
              // enough to support this new track

              Ogre::NodeAnimationTrack* track = anim->createNodeTrack(joint_id, bone);            
            
              for(uint32 key_it = 0; key_it < anim_key_frames.inputs.size() ; key_it++) {
                float32 key_time = anim_key_frames.inputs[key_it];

                anim->setLength( std::max(anim->getLength(), key_time) );
                Matrix4x4f mat =  anim_key_frames.outputs[key_it] * ibm * bsm;

                if (parent_id != 0) {
                  //need to cancel out the effect of BSM*IBM from the parent in the hierarchy
                  Matrix4x4f parentI_inv;
                  invert(parentI_inv, ibmMap[parent_id]);

                  mat = B_inv * parentI_inv * mat;
                }

                bool ret = getTRS(mat, translate, rotate, scale);
                assert(ret);

                if (ret ) {
                  //Set the transform for the keyframe.
                  Ogre::TransformKeyFrame* key = track->createNodeKeyFrame(key_time);

                  key->setTranslate( translate - startPos );

                  Ogre::Quaternion quat = startOrientation.Inverse() *  rotate;

                  key->setRotation( quat );
                  key->setScale( scale );
                }              
              }
            }
            else { //otherwise, just create a single keyframe using the node's transform.
              Ogre::NodeAnimationTrack* track = anim->createNodeTrack(joint_id, bone);            
              
              float32 key_time = 0;

              anim->setLength( std::max(anim->getLength(), key_time) );
              Matrix4x4f mat =  node.transform * ibm * bsm;

              if (parent_id != 0) {
                //need to cancel out the effect of BSM*IBM from the parent in the hierarchy
                Matrix4x4f parentI_inv;
                invert(parentI_inv, ibmMap[parent_id]);
                
                mat = B_inv * parentI_inv * mat;
              }
              
              bool ret = getTRS(mat, translate, rotate, scale);
              assert(ret);
              
              if (ret ) {
                //Set the transform for the keyframe.
                Ogre::TransformKeyFrame* key = track->createNodeKeyFrame(key_time);
                
                key->setTranslate( translate - startPos );
                
                Ogre::Quaternion quat = startOrientation.Inverse() *  rotate;
                
                key->setRotation( quat );
                key->setScale( scale );
              }
            }
          }
        }

        skeletonLoaded = true;
    }

    bool wasSkeletonLoaded() {
      return skeletonLoaded;
    }

private:
    MeshdataPtr mdptr;

    bool skeletonLoaded;

    const std::set<String>& animations;
};

class MeshdataManualLoader : public Ogre::ManualResourceLoader {
    MeshdataPtr mdptr;
    String meshname;
    Ogre::VertexData * createVertexData(const SubMeshGeometry &submesh, int vertexCount, Ogre::HardwareVertexBufferSharedPtr&vbuf) {

        using namespace Ogre;
        Ogre::VertexData *vertexData = OGRE_NEW Ogre::VertexData();
        VertexDeclaration* vertexDecl = vertexData->vertexDeclaration;
        size_t currOffset = 0;

        vertexDecl->addElement(0, currOffset, VET_FLOAT3, VES_POSITION);
        currOffset += VertexElement::getTypeSize(VET_FLOAT3);
        if (submesh.normals.size()==submesh.positions.size()) {
            vertexDecl->addElement(0, currOffset, VET_FLOAT3, VES_NORMAL);
            currOffset += VertexElement::getTypeSize(VET_FLOAT3);
        }
        if (submesh.tangents.size()==submesh.positions.size()) {
            vertexDecl->addElement(0, currOffset, VET_FLOAT3, VES_TANGENT);
            currOffset += VertexElement::getTypeSize(VET_FLOAT3);
        }
        if (submesh.colors.size()==submesh.positions.size()) {
            vertexDecl->addElement(0, currOffset, VET_COLOUR_ARGB, VES_DIFFUSE);
            currOffset += VertexElement::getTypeSize(VET_COLOUR_ARGB);
        }
        unsigned int numUVs=submesh.texUVs.size();
        for (unsigned int tc=0; tc<numUVs; ++ tc) {
            VertexElementType vet;
            switch (submesh.texUVs[tc].stride) {
              case 1:
                vet=VET_FLOAT1;
                break;
              case 2:
                vet=VET_FLOAT2;
                break;
              case 3:
                vet=VET_FLOAT3;
                break;
              case 4:
                vet=VET_FLOAT4;
                break;
              default:
                vet=VET_FLOAT4;
            }
            vertexDecl->addElement(0, currOffset, vet, VES_TEXTURE_COORDINATES, tc);
            currOffset += VertexElement::getTypeSize(vet);
        }
        vertexData->vertexCount = vertexCount;
        HardwareBuffer::Usage vertexBufferUsage= HardwareBuffer::HBU_STATIC_WRITE_ONLY;
        bool vertexShadowBuffer = false;
        vbuf = HardwareBufferManager::getSingleton().
            createVertexBuffer(vertexDecl->getVertexSize(0), vertexData->vertexCount,
                               vertexBufferUsage, vertexShadowBuffer);
        VertexBufferBinding* binding = vertexData->vertexBufferBinding;
        binding->setBinding(0, vbuf);
        return vertexData;
    }
public:

    MeshdataManualLoader(MeshdataPtr meshdata, const String& _meshname)
     :mdptr(meshdata),
      meshname(_meshname)
    {
    }

    void prepareResource(Ogre::Resource*r) {
    }

    void loadResource(Ogre::Resource *r) {
        size_t totalVertexCount;
        bool useSharedBuffer;

        getMeshStats(&useSharedBuffer, &totalVertexCount);
        traverseNodes(r, useSharedBuffer, totalVertexCount);
    }

    void getMeshStats(bool* useSharedBufferOut, size_t* totalVertexCountOut) {
        using namespace Ogre;

        const Meshdata& md = *mdptr;

        bool useSharedBuffer = true;
        for(GeometryInstanceList::const_iterator geoinst_it = md.instances.begin(); geoinst_it != md.instances.end(); geoinst_it++) {
            const GeometryInstance& geoinst = *geoinst_it;

            // Get the instanced submesh
            if (geoinst.geometryIndex >= md.geometry.size())
                continue;
            size_t i = geoinst.geometryIndex;

            if ((md.geometry[i].positions.size()==md.geometry[i].normals.size())
                != (md.geometry[0].positions.size()==md.geometry[0].normals.size())) {
                useSharedBuffer=false;
            }

            if ((md.geometry[i].positions.size()==md.geometry[i].tangents.size())
                != (md.geometry[0].positions.size()==md.geometry[0].tangents.size())) {
                useSharedBuffer=false;
            }

            if ((md.geometry[i].positions.size()==md.geometry[i].colors.size())
                != (md.geometry[0].positions.size()==md.geometry[0].colors.size())) {
                useSharedBuffer=false;
            }
            if (md.geometry[0].texUVs.size()!=md.geometry[i].texUVs.size()) {
                useSharedBuffer=false;
            } else {
                for (size_t j=0;j<md.geometry[0].texUVs.size();++j) {
                    if (md.geometry[i].texUVs[j].stride!=md.geometry[i].texUVs[j].stride)
                        useSharedBuffer=false;
                    if ((md.geometry[i].texUVs[j].uvs.size()==md.geometry[i].positions.size())!=
                        (md.geometry[0].texUVs[j].uvs.size()==md.geometry[0].positions.size())) {
                        useSharedBuffer=false;
                    }
                }
            }
        }

        // Be sure to compute over instantiated geometries. Otherwise
        // we would count extra for uninstantiated ones and compute
        // wrong for multiply instantiated geometries.
        size_t totalVertexCount=0;
        Meshdata::GeometryInstanceIterator geoinst_it = md.getGeometryInstanceIterator();
        uint32 geoinst_idx;
        Matrix4x4f pos_xform;
        while( geoinst_it.next(&geoinst_idx, &pos_xform) ) {
            totalVertexCount += md.geometry[ md.instances[geoinst_idx].geometryIndex ].positions.size();
        }

        if (totalVertexCount>65535)
            useSharedBuffer=false;

        *useSharedBufferOut = useSharedBuffer;
        *totalVertexCountOut = totalVertexCount;
    }


    void traverseNodes(Ogre::Resource* r, const bool useSharedBuffer, const size_t totalVertexCount) {
        using namespace Ogre;
        const Meshdata& md = *mdptr;

        Ogre::Mesh* mesh = dynamic_cast <Ogre::Mesh*> (r);

        if (totalVertexCount==0 || mesh==NULL)
            return;
        char * pData  = NULL;
        Ogre::HardwareVertexBufferSharedPtr vbuf;
        unsigned short sharedVertexOffset=0;
        unsigned int totalVerticesCopied=0;

        Ogre::VertexData* vertexData;

        bool hasBones = false;

        if (useSharedBuffer) {
            mesh->sharedVertexData = createVertexData(md.geometry[md.instances[0].geometryIndex],totalVertexCount, vbuf);
            pData=(char*)vbuf->lock(HardwareBuffer::HBL_DISCARD);
            vertexData = mesh->sharedVertexData;
        }

        Meshdata::GeometryInstanceIterator geoinst_it = md.getGeometryInstanceIterator();
        uint32 geoinst_idx;
        Matrix4x4f pos_xform;
        BoundingBox3f3f mesh_aabb = BoundingBox3f3f::null();
        double mesh_rad = 0.0;
        while( geoinst_it.next(&geoinst_idx, &pos_xform) ) {
            assert(geoinst_idx < md.instances.size());
            const GeometryInstance& geoinst = md.instances[geoinst_idx];

            Matrix3x3f normal_xform = pos_xform.extract3x3().inverseTranspose();

            // Get the instanced submesh
            if (geoinst.geometryIndex >= md.geometry.size())
                continue;
            const SubMeshGeometry& submesh = md.geometry[geoinst.geometryIndex];

            if (submesh.skinControllers.size() > 1) {
                SILOG(ogre,error,"Don't know how to handle multiple skin controllers, leaving in T-pose.");
            }
            else if (submesh.skinControllers.size() == 1) {
                hasBones = true;
                const SkinController& skin = submesh.skinControllers[0];
                // Set weights. This maps directly from our
                // (vert_idx,jointIndex,weight) pairs (at least as we
                // decode them) into Ogre.
                // FIXME this can be done on a per-submesh (rather
                // than per-mesh) basis. Do we ever need that?
                Ogre::VertexBoneAssignment vba;

                for(uint32 vidx = 0;  vidx < submesh.positions.size(); vidx++) {
                  vba.vertexIndex = vidx;

                  for(uint32 ass_idx = skin.weightStartIndices[vidx];  ass_idx < skin.weightStartIndices[vidx+1]; ass_idx++) {
                    vba.boneIndex = skin.joints[ skin.jointIndices[ass_idx] ]+1;
                    vba.weight = skin.weights[ass_idx];

                    mesh->addBoneAssignment(vba);
                  }
                }

                mesh->_compileBoneAssignments();
            }

            BoundingBox3f3f submeshaabb;
            double rad=0;
            geoinst.computeTransformedBounds(md, pos_xform, &submeshaabb, &rad);
            mesh_aabb = (mesh_aabb == BoundingBox3f3f::null() ? submeshaabb : mesh_aabb.merge(submeshaabb));
            mesh_rad = std::max(mesh_rad, rad);

            const SkinController* skin = (submesh.skinControllers.size() > 0) ? (&(submesh.skinControllers[0])) : NULL;
            if (skin != NULL && skin->bindShapeMatrix != Matrix4x4f::identity() && skin->bindShapeMatrix != Matrix4x4f::nil()) {
              rad = 0;
              submeshaabb = BoundingBox3f3f::null();
              geoinst.computeTransformedBounds(md, pos_xform*(skin->bindShapeMatrix), &submeshaabb, &rad);
              mesh_aabb = (mesh_aabb == BoundingBox3f3f::null() ? submeshaabb : mesh_aabb.merge(submeshaabb));
              mesh_rad = std::max(mesh_rad, rad);
            }

            int vertcount = submesh.positions.size();
            int normcount = submesh.normals.size();
            for (size_t primitive_index = 0; primitive_index<submesh.primitives.size(); ++primitive_index) {
                const SubMeshGeometry::Primitive& prim=submesh.primitives[primitive_index];

                // FIXME select proper texture/material
                GeometryInstance::MaterialBindingMap::const_iterator whichMaterial = geoinst.materialBindingMap.find(prim.materialId);
                if (whichMaterial == geoinst.materialBindingMap.end())
                    SILOG(ogre, error, "[OGRE] Invalid MaterialBindingMap: couldn't find " << prim.materialId << " for " << md.uri);
                std::string matname =
                    whichMaterial != geoinst.materialBindingMap.end() ?
                    meshname+"_mat_"+boost::lexical_cast<std::string>(whichMaterial->second) :
                    "baseogremat";
                Ogre::SubMesh *osubmesh = mesh->createSubMesh(submesh.name);

                osubmesh->setMaterialName(matname);
                if (useSharedBuffer) {
                    osubmesh->useSharedVertices=true;
                }else {
                    osubmesh->useSharedVertices=false;
                    osubmesh->vertexData = createVertexData(submesh,(int)submesh.positions.size(),vbuf);
                    pData=(char*)vbuf->lock(HardwareBuffer::HBL_DISCARD);
                    vertexData = osubmesh->vertexData;
                }

                if (primitive_index==0||useSharedBuffer==false) {
                    if (useSharedBuffer) {
                        sharedVertexOffset=totalVerticesCopied;
                        assert(totalVerticesCopied<65536||vertcount==0);//should be checked by other code
                    }
                    totalVerticesCopied+=vertcount;
                    bool warn_texcoords = false;
                    for (int i=0;i<vertcount; ++i) {
                        Vector3f v = submesh.positions[i];
                        Vector4f v_xform =  pos_xform * Vector4f(v[0], v[1], v[2], 1.f);
                        v = Vector3f(v_xform[0], v_xform[1], v_xform[2]);
                        memcpy(pData,&v.x,sizeof(float));
                        memcpy(pData+sizeof(float),&v.y,sizeof(float));
                        memcpy(pData+2*sizeof(float),&v.z,sizeof(float));
                        pData+=VertexElement::getTypeSize(VET_FLOAT3);
                        if (submesh.normals.size()==submesh.positions.size()) {
                            Vector3f normal = submesh.normals[i];
                            normal = (normal_xform * normal).normal();
                            memcpy(pData,&normal.x,sizeof(float));
                            memcpy(pData+sizeof(float),&normal.y,sizeof(float));
                            memcpy(pData+2*sizeof(float),&normal.z,sizeof(float));
                            pData+=VertexElement::getTypeSize(VET_FLOAT3);
                        }
                        if (submesh.tangents.size()==submesh.positions.size()) {
                            Vector3f tangent = submesh.tangents[i];
                            tangent = normal_xform * tangent;
                            memcpy(pData,&tangent.x,sizeof(float));
                            memcpy(pData+sizeof(float),&tangent.y,sizeof(float));
                            memcpy(pData+2*sizeof(float),&tangent.z,sizeof(float));
                            pData+=VertexElement::getTypeSize(VET_FLOAT3);
                        }
                        if (submesh.colors.size()==submesh.positions.size()) {
                            unsigned char r = (unsigned char)(submesh.colors[i].x*255);
                            unsigned char g = (unsigned char)(submesh.colors[i].y*255);
                            unsigned char b = (unsigned char)(submesh.colors[i].z*255);
                            unsigned char a = (unsigned char)(submesh.colors[i].w*255);
                            pData[0]=a;
                            pData[1]=r;
                            pData[2]=g;
                            pData[3]=b;
                            pData+=VertexElement::getTypeSize(VET_COLOUR_ARGB);
                        }
                        unsigned int numUVs= submesh.texUVs.size();
                        for (unsigned int tc=0;tc<numUVs;++tc) {
                            VertexElementType vet;
                            int stride=0;
                            switch (submesh.texUVs[tc].stride) {
                              case 1:
                                stride=1;
                                vet=VET_FLOAT1;
                                break;
                              case 2:
                                stride=2;
                                vet=VET_FLOAT2;
                                break;
                              case 3:
                                stride=3;
                                vet=VET_FLOAT3;
                            break;
                              case 4:
                              default:
                                stride=4;
                                vet=VET_FLOAT4;
                                break;
                            }

                            // This should be:
                            //assert( i*stride < submesh.texUVs[tc].uvs.size() );
                            // but so many models seem to get this
                            // wrong that we need to hack around it.
                            if ( i*stride < (int)submesh.texUVs[tc].uvs.size() )
                                memcpy(pData,&submesh.texUVs[tc].uvs[i*stride],sizeof(float)*stride);
                            else { // The hack: just zero out the data
                                warn_texcoords = true;
                                memset(pData, 0, sizeof(float)*stride);
                            }

                            if (submesh.texUVs[tc].stride > 1) {
                                // Apparently we need to invert the V
                                // coordinate... perhaps somebody could document
                                // why we need this.
                                float UVHACK = submesh.texUVs[tc].uvs[i*stride+1];
                                UVHACK=1.0-UVHACK;
                                memcpy(pData+sizeof(float),&UVHACK,sizeof(float));
                            }

                            pData += VertexElement::getTypeSize(vet);
                        }
                    }
                    if (warn_texcoords) {
                        SILOG(ogre,warn,"Out of bounds texture coordinates on " << md.uri);
                    }
                    if (!useSharedBuffer) {
                        vbuf->unlock();

                        if (hasBones) {
                          Ogre::VertexDeclaration* newDecl = osubmesh->vertexData->vertexDeclaration->getAutoOrganisedDeclaration(true, false);
                          osubmesh->vertexData->reorganiseBuffers(newDecl);
                        }
                    }
                }

                unsigned int indexcount = prim.indices.size();
                osubmesh->indexData->indexCount = indexcount;
                HardwareBuffer::Usage indexBufferUsage= HardwareBuffer::HBU_STATIC_WRITE_ONLY;
                bool indexShadowBuffer = false;

                osubmesh->indexData->indexBuffer = HardwareBufferManager::getSingleton().
                    createIndexBuffer(HardwareIndexBuffer::IT_16BIT,
                                      indexcount, indexBufferUsage, indexShadowBuffer);
                unsigned short * iData = (unsigned short*)osubmesh->indexData->indexBuffer->lock(HardwareBuffer::HBL_DISCARD);
                if (useSharedBuffer) {
                    for (unsigned int i=0;i<indexcount;++i) {
                        iData[i]=prim.indices[i]+sharedVertexOffset;
                        assert(iData[i]<totalVertexCount);
                    }
                }else {
                    memcpy(iData,&prim.indices[0],indexcount*sizeof(unsigned short));
                }
                osubmesh->indexData->indexBuffer->unlock();
                switch (prim.primitiveType) {
                  case SubMeshGeometry::Primitive::TRIANGLES:
                    osubmesh->operationType = Ogre::RenderOperation::OT_TRIANGLE_LIST;
                    break;
                  case SubMeshGeometry::Primitive::TRISTRIPS:
                    osubmesh->operationType = Ogre::RenderOperation::OT_TRIANGLE_STRIP;
                    break;
                  case SubMeshGeometry::Primitive::TRIFANS:
                    osubmesh->operationType = Ogre::RenderOperation::OT_TRIANGLE_FAN;
                    break;
                  case SubMeshGeometry::Primitive::LINESTRIPS:
                    osubmesh->operationType = Ogre::RenderOperation::OT_LINE_STRIP;
                    break;
                  case SubMeshGeometry::Primitive::LINES:
                    osubmesh->operationType = Ogre::RenderOperation::OT_LINE_LIST;
                    break;
                  case SubMeshGeometry::Primitive::POINTS:
                    osubmesh->operationType = Ogre::RenderOperation::OT_POINT_LIST;
                    break;
                  default:
                    break;
                }
            }
        }

        AxisAlignedBox ogremeshaabb(
            Graphics::toOgre(mesh_aabb.min()),
            Graphics::toOgre(mesh_aabb.max())
        );
        mesh->_setBounds(ogremeshaabb);
        mesh->_setBoundingSphereRadius(mesh_rad);

        if (useSharedBuffer) {
            assert(totalVerticesCopied==totalVertexCount);

            vbuf->unlock();

            if (hasBones) {
                 Ogre::VertexDeclaration* newDecl = vertexData->vertexDeclaration->getAutoOrganisedDeclaration(true, false);
                 vertexData->reorganiseBuffers(newDecl);
            }
        }
        // This doesn't seem to be required in Ogre 1.7, and in fact seems to
        // trigger a recursive dead-lock. Since I can't find documentation about
        // this change, I'm leaving this in for the time being.
        // mesh->load();
    }
};

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

                (*mTextureFingerprints)[tex_data.request->getURI().toString()] = id;

                fixOgreURI(id);
                CDNArchiveFactory::getSingleton().addArchiveData(mCDNArchive,id,SparseData(tex_data.response));
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
            std::string matname = meshname+"_mat_"+boost::lexical_cast<std::string>(index);
            Ogre::MaterialPtr matPtr=matm.getByName(matname);
            if (matPtr.isNull()) {
                Ogre::ManualResourceLoader * reload;
                matPtr = matm.create(
                    matname, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, true,
                    (reload=new MaterialManualLoader (mdptr, matname, *mat, mURI, mTextureFingerprints))
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
            skel = Ogre::SkeletonPtr(skel_mgr.create(meshname+"_skeleton",Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, true,
                                                     (reload=new SkeletonManualLoader(mdptr, mAnimationList))));
            reload->prepareResource(&*skel);
            reload->loadResource(&*skel);

            if (! ((SkeletonManualLoader*)reload)->wasSkeletonLoaded()) {
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
                        MeshdataManualLoader(mdptr, meshname))));
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

        String lightname = mName+"_light_"+meshname+ boost::lexical_cast<String>(light_idx++);
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

    if (!mInitialAnimationName.empty()) {
      setAnimation(mInitialAnimationName);
      mInitialAnimationName = "";
    }
}

void Entity::createBillboard(const BillboardPtr& bbptr, bool usingDefault, AssetDownloadTaskPtr assetDownload) {
    SHA256 sha = bbptr->hash;
    String hash = sha.convertToHexString();
    String bbname = computeVisualHash(bbptr, assetDownload).toString();

    loadDependentTextures(assetDownload, usingDefault);

    // Load the single material
    Ogre::MaterialManager& matm = Ogre::MaterialManager::getSingleton();

    std::string matname = bbname + "_mat_billboard_";
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
        tex.affecting = MaterialEffectInfo::Texture::DIFFUSE;
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
            (reload = new MaterialManualLoader (bbptr, matname, matinfo, mURI, mTextureFingerprints))
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

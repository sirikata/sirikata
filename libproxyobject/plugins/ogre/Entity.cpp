/*  Sirikata Graphical Object Host
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

#include <sirikata/proxyobject/Platform.hpp>
#include "Entity.hpp"
#include <sirikata/core/options/Options.hpp>
#include "OgreSystem.hpp"
#include "resourceManager/CDNArchive.hpp"
#include "OgreHeaders.hpp"
#include "resourceManager/ResourceDownloadTask.hpp"
#include "Lights.hpp"

using namespace Sirikata::Transfer;

namespace Sirikata {
namespace Graphics {

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

Entity::Entity(OgreSystem *scene,
    const ProxyObjectPtr &ppo)
  : mScene(scene),
    mProxy(ppo),
    mOgreObject(NULL),
    mSceneNode(scene->getSceneManager()->createSceneNode( ogreMeshName(ppo->getObjectReference()) )),
    mMovingIter(scene->mMovingEntities.end())
{
    mSceneNode->setInheritScale(false);
    addToScene(NULL);

    bool successful = scene->mSceneEntities.insert(
        OgreSystem::SceneEntitiesMap::value_type(mProxy->getObjectReference(), this)).second;

    assert (successful == true);

    ppo->ProxyObjectProvider::addListener(this);
    ppo->PositionProvider::addListener(this);


    mRemainingDownloads = 0;
    mCDNArchive=CDNArchiveFactory::getSingleton().addArchive();
    mActiveCDNArchive=true;
    getProxy().MeshProvider::addListener(this);
    unloadMesh();
}

Entity::~Entity() {
    Ogre::Entity * toDestroy=getOgreEntity();
    init(NULL);
    if (toDestroy) {
        getScene()->getSceneManager()->destroyEntity(toDestroy);
    }
    getProxy().MeshProvider::removeListener(this);


    OgreSystem::SceneEntitiesMap::iterator iter =
        mScene->mSceneEntities.find(mProxy->getObjectReference());
    if (iter != mScene->mSceneEntities.end()) {
        // will fail while in the OgreSystem destructor.
        mScene->mSceneEntities.erase(iter);
    }
    if (mMovingIter != mScene->mMovingEntities.end()) {
        mScene->mMovingEntities.erase(mMovingIter);
        mMovingIter = mScene->mMovingEntities.end();
    }
    getProxy().ProxyObjectProvider::removeListener(this);
    getProxy().PositionProvider::removeListener(this);
    removeFromScene();
    init(NULL);
    mSceneNode->detachAllObjects();
    /* detaches all children from the scene.
       There should be none, as the server should have adjusted their parents.
     */
    mSceneNode->removeAllChildren();
    mScene->getSceneManager()->destroySceneNode(mSceneNode);
}

std::string Entity::ogreMeshName(const SpaceObjectReference&ref) {
    return "Mesh:"+ref.toString();
}
std::string Entity::ogreMovableName()const{
    return ogreMeshName(id());
}

Entity *Entity::fromMovableObject(Ogre::MovableObject *movable) {
    return Ogre::any_cast<Entity*>(movable->getUserAny());
}

void Entity::init(Ogre::Entity *obj) {
    if (mOgreObject) {
        mSceneNode->detachObject(mOgreObject);
    }
    mOgreObject = obj;
    if (obj) {
        mOgreObject->setUserAny(Ogre::Any(this));
        mSceneNode->attachObject(obj);
        updateScale( getProxy().getBounds().radius() );
    }
}

void Entity::updateScale(float scale) {
    if (mSceneNode == NULL || mOgreObject == NULL) return;
    mSceneNode->setScale( 1.f, 1.f, 1.f );
    float rad = mOgreObject->getBoundingRadius();
    float rad_factor = scale / rad;
    mSceneNode->setScale( rad_factor, rad_factor, rad_factor );
}

void Entity::setStatic(bool isStatic) {
    const std::list<Entity*>::iterator end = mScene->mMovingEntities.end();
    if (isStatic) {
        if (mMovingIter != end) {
            SILOG(ogre,debug,"Removed "<<this<<" from moving entities queue.");
            mScene->mMovingEntities.erase(mMovingIter);
            mMovingIter = end;
        }
    } else {
        if (mMovingIter == end) {
            SILOG(ogre,debug,"Added "<<this<<" to moving entities queue.");
            mMovingIter = mScene->mMovingEntities.insert(end, this);
        }
    }
}

void Entity::setVisible(bool vis) {
    mSceneNode->setVisible(vis, true);
}

void Entity::removeFromScene() {
    Ogre::SceneNode *oldParent = mSceneNode->getParentSceneNode();
    if (oldParent) {
        oldParent->removeChild(mSceneNode);
    }
    setStatic(true);
}
void Entity::addToScene(Ogre::SceneNode *newParent) {
    if (newParent == NULL) {
        newParent = mScene->getSceneManager()->getRootSceneNode();
    }
    removeFromScene();
    newParent->addChild(mSceneNode);
    setStatic(false); // May get set to true after the next frame has drawn.
}

void Entity::setOgrePosition(const Vector3d &pos) {
    //SILOG(ogre,debug,"setOgrePosition "<<this<<" to "<<pos);
    Ogre::Vector3 ogrepos = toOgre(pos, getScene()->getOffset());
    const Ogre::Vector3 &scale = mSceneNode->getScale();
    mSceneNode->setPosition(ogrepos);
}

void Entity::setOgreOrientation(const Quaternion &orient) {
    //SILOG(ogre,debug,"setOgreOrientation "<<this<<" to "<<orient);
    mSceneNode->setOrientation(toOgre(orient));
}


void Entity::updateLocation(const TimedMotionVector3f &newLocation, const TimedMotionQuaternion& newOrient, const BoundingSphere3f& newBounds) {
    SILOG(ogre,debug,"UpdateLocation "<<this<<" to "<<newLocation.position()<<"; "<<newOrient.position());
    if (!getProxy().isStatic()) {
        setStatic(false);
    } else {
        setOgrePosition(Vector3d(newLocation.position()));
        setOgreOrientation(newOrient.position());
    }
    updateScale( newBounds.radius() );
}

void Entity::destroyed() {
    delete this;
}
void Entity::extrapolateLocation(TemporalValue<Location>::Time current) {
    Location loc (getProxy().extrapolateLocation(current));
    setOgrePosition(loc.getPosition());
    setOgreOrientation(loc.getOrientation());
    setStatic(getProxy().isStatic());
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
	SILOG(ogre,debug,"Fixing texture for "<<id());
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
		SILOG(ogre,debug,"Resetting a material "<<id());
	}
	mReplacedMaterials.clear();
	for (int whichSubEntity = 0; whichSubEntity < numSubEntities; whichSubEntity++) {
		Ogre::SubEntity *subEnt = ent->getSubEntity(whichSubEntity);
		Ogre::MaterialPtr material = subEnt->getMaterial();
		if (forEachTexture(material, ShouldReplaceTexture(mTextureBindings))) {
			SILOG(ogre,debug,"Replacing a material "<<id());
			Ogre::MaterialPtr newMaterial = material->clone(material->getName()+id().toString(), false, Ogre::String());
			String newTexture;
			for (TextureBindingsMap::const_iterator iter = mTextureBindings.begin();
					iter != mTextureBindings.end();
					++iter) {
				SILOG(ogre,debug,"Replacing a texture "<<id()<<" : "<<iter->first<<" -> "<<iter->second);
				forEachTexture(newMaterial, ReplaceTexture(iter->first, iter->second));
				newTexture = iter->second;
			}
			mReplacedMaterials.insert(
				ReplacedMaterialMap::value_type(whichSubEntity, std::pair<String, Ogre::MaterialPtr>(newTexture, newMaterial)));
			subEnt->setMaterial(newMaterial);
		}
	}
}

void Entity::bindTexture(const std::string &textureName, const SpaceObjectReference &objId) {
	mTextureBindings[textureName] = objId.toString();
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
    unloadMesh();

    mReplacedMaterials.clear();

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
      } catch (Ogre::InvalidParametersException &) {
        SILOG(ogre,error,"Got invalid parameters");
        throw;
      }
    } catch (...) {
        SILOG(ogre,error,"Failed to load mesh "<<getProxy().getMesh()<< " (id "<<id()<<")!");

        return;
        //new_entity = getScene()->getSceneManager()->createEntity(ogreMovableName(),Ogre::SceneManager::PT_CUBE);
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
    fixTextures();
    if (mActiveCDNArchive) {
        CDNArchiveFactory::getSingleton().removeArchive(mCDNArchive);
        mActiveCDNArchive=false;
    }else {
        SILOG(ogre,error,"Archive deactivated early, texture data may be unavailable");
    }
}

void Entity::unloadMesh() {
    Ogre::Entity * meshObj=getOgreEntity();
    //init(getScene()->getSceneManager()->createEntity(ogreMovableName(), Ogre::SceneManager::PT_CUBE));
    init(NULL);
    if (meshObj) {
        getScene()->getSceneManager()->destroyEntity(meshObj);
    }
    mReplacedMaterials.clear();
}

void Entity::setSelected(bool selected) {
      mSceneNode->showBoundingBox(selected);
}

void Entity::MeshDownloaded(std::tr1::shared_ptr<ChunkRequest>request, std::tr1::shared_ptr<const DenseData> response)
{
    mScene->context()->mainStrand->post(std::tr1::bind(&Entity::tryInstantiateExistingMesh, this, request, response));
}

void Entity::downloadMeshFile(Transfer::URI const& uri)
{
    ResourceDownloadTask *dl = new ResourceDownloadTask(
        uri, getScene()->transferPool(),
        mProxy->priority,
        std::tr1::bind(&Entity::MeshDownloaded,
            this, std::tr1::placeholders::_1, std::tr1::placeholders::_2));
    (*dl)();
}


/////////////////////////////////////////////////////////////////////
// overrides from MeshListener
// MCB: integrate these with the MeshObject model class

void Entity::onSetMesh (ProxyObjectPtr proxy, Transfer::URI const& meshFile )
{

}

void Entity::processMesh(Transfer::URI const& meshFile)
{
    Ogre::Entity * meshObj=getOgreEntity();

    if (meshObj && meshFile.filename() == "" ) {
        setVisible(false);
        return;
    }
    else if (meshObj) {
        setVisible(true);
        return;
    }

    mURI = meshFile;
    mURIString = meshFile.toString();

    downloadMeshFile(meshFile);
}

bool Entity::createMeshWork(MeshdataPtr md) {
    createMesh(md);
    return true;
}
Ogre::TextureUnitState::TextureAddressingMode translateWrapMode(MaterialEffectInfo::Texture::WrapMode w) {
    switch(w) {
      case MaterialEffectInfo::Texture::WRAP_MODE_CLAMP:
        printf ("CLAMPING");
        return Ogre::TextureUnitState::TAM_CLAMP;
      case MaterialEffectInfo::Texture::WRAP_MODE_MIRROR:
        printf ("MIRRORING");
        return Ogre::TextureUnitState::TAM_MIRROR;
      case MaterialEffectInfo::Texture::WRAP_MODE_WRAP:
      default:
        printf ("WRAPPING");
        return Ogre::TextureUnitState::TAM_WRAP;
    }
}

void fixupTextureUnitState(Ogre::TextureUnitState*tus, const MaterialEffectInfo::Texture&tex) {
    tus->setTextureAddressingMode(translateWrapMode(tex.wrapS),
                                  translateWrapMode(tex.wrapT),
                                  translateWrapMode(tex.wrapU));

}
class MaterialManualLoader : public Ogre::ManualResourceLoader {
    MaterialEffectInfo mMat;
    std::string mName;
    String mURI;
    Entity::TextureBindingsMap mTextureFingerprints;
public:
    MaterialManualLoader(const std::string &name,
                         const MaterialEffectInfo&mat,
                         const std::string uri,
                         const Entity::TextureBindingsMap& textureFingerprints):mTextureFingerprints(textureFingerprints) {
        mName=name;
        mMat=mat;
        mURI=uri;
    }
    void prepareResource(Ogre::Resource*r){}
    void loadResource(Ogre::Resource *r) {
        using namespace Ogre;
        Material* material= dynamic_cast <Material*> (r);
        material->setCullingMode(CULL_NONE);
        Ogre::Technique* tech=material->getTechnique(0);
        bool useAlpha=false;
        if (mMat.textures.empty()) {
            Ogre::Pass*pass=tech->getPass(0);
            pass->setDiffuse(ColourValue(1,1,1,1));
            pass->setAmbient(ColourValue(1,1,1,1));
            pass->setSelfIllumination(ColourValue(0,0,0,0));
            pass->setSpecular(ColourValue(1,1,1,1));
        }
        for (size_t i=0;i<mMat.textures.size();++i) {
            if (mMat.textures[i].affecting==MaterialEffectInfo::Texture::OPACITY&&
                (mMat.textures[i].uri.length()||mMat.textures[i].color.w<1.0)){
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
        for (size_t i=0;i<mMat.textures.size();++i) {
            MaterialEffectInfo::Texture&tex=mMat.textures[i];
            Ogre::Pass*pass=tech->getPass(0);
/*
            if (tex.uri.length()&&tech->getNumPasses()<=valid_passes) {
                pass=tech->createPass();
                ++valid_passes;
            }
*/
            if (tex.uri.length()==0) {
                switch (tex.affecting) {
                case MaterialEffectInfo::Texture::DIFFUSE:
                  pass->setDiffuse(ColourValue(tex.color.x,
                                               tex.color.y,
                                               tex.color.z,
                                               tex.color.w));
/*
                  pass->setAmbient(ColourValue(0,0,0,0));
                  pass->setSelfIllumination(ColourValue(0,0,0,0));
                  pass->setSpecular(ColourValue(0,0,0,0));
*/
                  break;
                case MaterialEffectInfo::Texture::AMBIENT:
/*
                  pass->setDiffuse(ColourValue(0,0,0,0));
                  pass->setSelfIllumination(ColourValue(0,0,0,0));
                  pass->setSpecular(ColourValue(0,0,0,0));
*/
                  pass->setAmbient(ColourValue(tex.color.x,
                                               tex.color.y,
                                               tex.color.z,
                                               tex.color.w));
                  break;
                case MaterialEffectInfo::Texture::EMISSION:
/*
                  pass->setDiffuse(ColourValue(0,0,0,0));
                  pass->setAmbient(ColourValue(0,0,0,0));
                  pass->setSpecular(ColourValue(0,0,0,0));
*/
                  pass->setSelfIllumination(ColourValue(tex.color.x,
                                                        tex.color.y,
                                                        tex.color.z,
                                                        tex.color.w));
                  break;
                case MaterialEffectInfo::Texture::SPECULAR:
/*
                  pass->setDiffuse(ColourValue(0,0,0,0));
                  pass->setAmbient(ColourValue(0,0,0,0));
                  pass->setSelfIllumination(ColourValue(0,0,0,0));

                  pass->setSpecular(ColourValue(tex.color.x,
                                                tex.color.y,
                                                tex.color.z,
                                                tex.color.w));
*///FIXME looks awful for some reason
                  break;
                default:
                  break;
                }
            }else if (tex.affecting==MaterialEffectInfo::Texture::DIFFUSE) {
                String texURI = mURI.substr(0, mURI.rfind("/")+1) + tex.uri;
                Entity::TextureBindingsMap::iterator where = mTextureFingerprints.find(texURI);
                if (where!=mTextureFingerprints.end()) {
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
                        tus=pass->createTextureUnitState(ogreTextureName,tex.texCoord);
                        fixupTextureUnitState(tus,tex);
                        tus->setColourOperation(LBO_MODULATE);

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
                        tus->setColourOperation(LBO_MODULATE);

                        pass->setIlluminationStage(IS_AMBIENT);
                        break;
                      case MaterialEffectInfo::Texture::EMISSION:
/*
                        pass->setDiffuse(ColourValue(0,0,0,0));
                        pass->setAmbient(ColourValue(0,0,0,0));
                        pass->setSpecular(ColourValue(0,0,0,0));
*/
                        tus=pass->createTextureUnitState(ogreTextureName,tex.texCoord);
                        fixupTextureUnitState(tus,tex);
//                        pass->setSelfIllumination(ColourValue(1,1,1,1));
                        tus->setColourOperation(LBO_ADD);

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

                        tus=pass->createTextureUnitState(ogreTextureName,tex.texCoord);
                        fixupTextureUnitState(tus,tex);
                        tus->setColourOperation(LBO_MODULATE);
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


class MeshdataManualLoader : public Ogre::ManualResourceLoader {
    Meshdata md;
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

    MeshdataManualLoader(const Meshdata&meshdata):md(meshdata) {

    }
    void prepareResource(Ogre::Resource*r){}
    void loadResource(Ogre::Resource *r) {
        using namespace Ogre;
        SHA256 sha = md.hash;
        String hash = sha.convertToHexString();
        bool useSharedBuffer = true;
        size_t totalVertexCount=0;
        for(Meshdata::GeometryInstanceList::const_iterator geoinst_it = md.instances.begin(); geoinst_it != md.instances.end(); geoinst_it++) {
            const GeometryInstance& geoinst = *geoinst_it;

            // Get the instanced submesh
            if (geoinst.geometryIndex >= md.geometry.size())
                continue;
            size_t i = geoinst.geometryIndex;

            totalVertexCount+=md.geometry[i].positions.size();
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

        if (totalVertexCount>65535)
            useSharedBuffer=false;
        Mesh* mesh= dynamic_cast <Mesh*> (r);

        if (totalVertexCount==0 || mesh==NULL)
            return;
        char * pData  = NULL;
        Ogre::HardwareVertexBufferSharedPtr vbuf;
        unsigned short sharedVertexOffset=0;
        unsigned int totalVerticesCopied=0;
        if (useSharedBuffer) {
            mesh->sharedVertexData = createVertexData(md.geometry[md.instances[0].geometryIndex],totalVertexCount, vbuf);
            pData=(char*)vbuf->lock(HardwareBuffer::HBL_DISCARD);
        }
        for(Meshdata::GeometryInstanceList::const_iterator geoinst_it = md.instances.begin(); geoinst_it != md.instances.end(); geoinst_it++) {
            const GeometryInstance& geoinst = *geoinst_it;

            Matrix4x4f pos_xform = geoinst.transform;
            Matrix3x3f normal_xform = pos_xform.extract3x3().inverseTranspose();

            // Get the instanced submesh
            if (geoinst.geometryIndex >= md.geometry.size())
                continue;
            const SubMeshGeometry& submesh = md.geometry[geoinst.geometryIndex];
            AxisAlignedBox ogresubmeshaabb(
                Graphics::toOgre(geoinst.aabb.min()),
                Graphics::toOgre(geoinst.aabb.max())
            );
            double rad=0;
            if (geoinst_it != md.instances.begin()) {
                ogresubmeshaabb.merge(mesh->getBounds());
                rad=mesh->getBoundingSphereRadius();
            }
            if (geoinst.radius>rad) {
                rad=geoinst.radius;
            }
            mesh->_setBounds(ogresubmeshaabb);
            mesh->_setBoundingSphereRadius(rad);
            int vertcount = submesh.positions.size();
            int normcount = submesh.normals.size();
            for (size_t primitive_index = 0; primitive_index<submesh.primitives.size(); ++primitive_index) {
                const SubMeshGeometry::Primitive& prim=submesh.primitives[primitive_index];

                // FIXME select proper texture/material
                GeometryInstance::MaterialBindingMap::const_iterator whichMaterial = geoinst.materialBindingMap.find(prim.materialId);
                std::string matname = whichMaterial!=geoinst.materialBindingMap.end()?hash+"_mat_"+boost::lexical_cast<std::string>(whichMaterial->second):"baseogremat";
                Ogre::SubMesh *osubmesh = mesh->createSubMesh(submesh.name);

                osubmesh->setMaterialName(matname);
                if (useSharedBuffer) {
                    osubmesh->useSharedVertices=true;
                }else {
                    osubmesh->useSharedVertices=false;
                    osubmesh->vertexData = createVertexData(submesh,(int)submesh.positions.size(),vbuf);
                    pData=(char*)vbuf->lock(HardwareBuffer::HBL_DISCARD);
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
                        Vector4f v_xform = pos_xform * Vector4f(v[0], v[1], v[2], 1.f);
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

                            float UVHACK = submesh.texUVs[tc].uvs[i*stride+1];
                            UVHACK=1.0-UVHACK;
                            memcpy(pData+sizeof(float),&UVHACK,sizeof(float));
                            pData += VertexElement::getTypeSize(vet);
                        }
                    }
                    if (warn_texcoords) {
                        SILOG(ogre,warn,"Out of bounds texture coordinates on " << md.uri);
                    }
                    if (!useSharedBuffer)
                        vbuf->unlock();
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
        if (useSharedBuffer) {
            assert(totalVerticesCopied==totalVertexCount);

            vbuf->unlock();
        }
        mesh->load();
    }
};

bool Entity::tryInstantiateExistingMesh(Transfer::ChunkRequestPtr request, DenseDataPtr response) {
    SHA256 sha = request->getMetadata().getFingerprint();
    String hash = sha.convertToHexString();
    Ogre::MeshPtr mp = Ogre::MeshManager::getSingleton().getByName(hash);
    if (!mp.isNull()) {
        loadMesh(hash);
    }
    else {
        // Otherwise, follow the rest of the normal process.
        MeshdataPtr mesh_data = mScene->parseMesh(mURI, request->getMetadata().getFingerprint(), response);
        if (!mesh_data)
        {
            SILOG(ogre,error,"Failed to parse mesh " << mURI.toString() << " --> " << request->getMetadata().getFingerprint().toString());
            return true;
        }


        handleMeshParsed(mesh_data);
    }
    return true;
}

void Entity::createMesh(MeshdataPtr mdptr) {
    const Meshdata& md = *mdptr;

    SHA256 sha = md.hash;
    String hash = sha.convertToHexString();

    if (!md.instances.empty()) {
        Ogre::MaterialManager& matm = Ogre::MaterialManager::getSingleton();
        int index=0;
        for (Meshdata::MaterialEffectInfoList::const_iterator mat=md.materials.begin(),mate=md.materials.end();mat!=mate;++mat,++index) {
            std::string matname = hash+"_mat_"+boost::lexical_cast<string>(index);
            Ogre::MaterialPtr matPtr=matm.getByName(matname);
            if (matPtr.isNull()) {
                Ogre::ManualResourceLoader * reload;
                matPtr=matm.create(matname,Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,true,(reload=new MaterialManualLoader (matname,*mat, mURIString, mTextureFingerprints)));

                reload->prepareResource(&*matPtr);
                reload->loadResource(&*matPtr);

            }
        }
        Ogre::MaterialPtr base_mat = matm.getByName("baseogremat");
        for(Meshdata::TextureList::const_iterator tex_it = md.textures.begin(); tex_it != md.textures.end(); tex_it++){
          std::string matname = hash + "_texture_" + (*tex_it);
            Ogre::MaterialPtr mat = base_mat->clone(matname);
            String texURI = mURIString.substr(0, mURIString.rfind("/")+1) + (*tex_it);
            String ogreTextureName = "Cache/" + mTextureFingerprints[texURI];
            mat->getTechnique(0)->getPass(0)->createTextureUnitState(ogreTextureName,0);
        }
        Ogre::MeshManager& mm = Ogre::MeshManager::getSingleton();
        if (1) {
            /// FIXME: set bounds, bounding radius here
            Ogre::ManualResourceLoader *reload;
            Ogre::MeshPtr mo (mm.createManual(hash,Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,(reload=OGRE_NEW MeshdataManualLoader(md))));
            reload->prepareResource(&*mo);
            reload->loadResource(&*mo);
        }else {
            /// FIXME: set bounds, bounding radius here
            Ogre::ManualObject mo(hash);
            mo.clear();

            for(Meshdata::GeometryInstanceList::const_iterator geoinst_it = md.instances.begin(); geoinst_it != md.instances.end(); geoinst_it++) {
                const GeometryInstance& geoinst = *geoinst_it;

                Matrix4x4f pos_xform = geoinst.transform;
                Matrix3x3f normal_xform = pos_xform.extract3x3().inverseTranspose();

                // Get the instanced submesh
                assert(geoinst.geometryIndex < md.geometry.size());
                const SubMeshGeometry& submesh = md.geometry[geoinst.geometryIndex];

                int vertcount = submesh.positions.size();
                int normcount = submesh.normals.size();
                for (size_t primitive_index = 0; primitive_index<submesh.primitives.size(); ++primitive_index) {
                    const SubMeshGeometry::Primitive& prim=submesh.primitives[primitive_index];
                    int indexcount = prim.indices.size();

                    // FIXME select proper texture/material
                    std::string matname = md.textures.size() > 0 ?
                        hash + "_texture_" + md.textures[0] :
                        base_mat->getName();
                    mo.begin(matname);
                    std::cerr<<"Begin "<<matname<<"\n";
                    float tu, tv;
                    for (int i=0; i<indexcount; i++) {
                        int j = prim.indices[i];
                        Vector3f v = submesh.positions[j];
                        Vector4f v_xform = pos_xform * Vector4f(v[0], v[1], v[2], 1.f);
                        v = Vector3f(v_xform[0], v_xform[1], v_xform[2]);
                        mo.position(v[0], v[1], v[2]);
                        std::cerr<<"Mo pos "<<v[0]<<","<<v[1]<<","<<v[2]<<"\n";
                        Vector3f normal = submesh.normals[j];
                        normal = normal_xform * normal;
                        mo.normal(normal[0], normal[1], normal[2]);
                        std::cerr<<"Mo norm "<<normal[0]<<","<<normal[1]<<","<<normal[2]<<"\n";
                        mo.colour(1.0,1.0,1.0,1.0);
                        if (submesh.texUVs.size()==0) {
                            /// bogus texture for textureless models
                            if (i%3==0) {
                                tu=0.0;
                                tv=0.0;
                            }
                            if (i%3==1) {
                                tu=0.0;
                                tv=1.0;
                            }
                            if (i%3==2) {
                                tu=1.0;
                                tv=1.0;
                            }
                        }
                        else {
                            Sirikata::Vector2f uv (submesh.texUVs[0].uvs[ j*submesh.texUVs[0].stride ],
                                                   submesh.texUVs[0].uvs[ j*submesh.texUVs[0].stride+1]);
                            tu = uv[0];
                            tv = 1.0-uv[1];           //  why you gotta be like that?
                        }
                        std::cerr<<"Mo tex "<<tu<<","<<tv<<"\n";
                        mo.textureCoord(tu, tv);
                    }
                    std::cerr<<"Mo end\n";
                    mo.end();
                }
            } // submesh

            mo.setVisible(true);
            Ogre::MeshPtr mp = mo.convertToMesh(hash, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
        }

        bool check = mm.resourceExists(hash);

        loadMesh(hash);
    }
    // Lights
    int light_idx = 0;
    for(Meshdata::LightInstanceList::const_iterator lightinst_it = md.lightInstances.begin(); lightinst_it != md.lightInstances.end(); lightinst_it++) {
        const LightInstance& lightinst = *lightinst_it;

        Matrix4x4f pos_xform = lightinst.transform;

        // Get the instanced submesh
        if(lightinst.lightIndex >= (int)md.lights.size()){
            SILOG(ogre,error, "bad light index %d for lights only sized %d\n"<<lightinst.lightIndex<<"/"<<(int)md.lights.size());
            continue;
        }
        const LightInfo& sublight = md.lights[lightinst.lightIndex];

        String lightname = getProxy().getObjectReference().toString()+"_light_"+hash+ boost::lexical_cast<String>(light_idx++);
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

void Entity::downloadFinished(std::tr1::shared_ptr<ChunkRequest> request,
    std::tr1::shared_ptr<const DenseData> response, MeshdataPtr md) {

    mTextureFingerprints[request->getURI().toString()] = request->getIdentifier();
    if (mActiveCDNArchive) {
        String id = request->getIdentifier();
        fixOgreURI(id);
        CDNArchiveFactory::getSingleton().addArchiveData(mCDNArchive,id,SparseData(response));
    }
    mRemainingDownloads--;
    if (mRemainingDownloads == 0)
        mScene->context()->mainStrand->post(std::tr1::bind(&Entity::createMeshWork, this, md));
}

void Entity::handleMeshParsed(MeshdataPtr md) {
    mRemainingDownloads += md->textures.size();

    // Special case for no dependent downloads
    if (mRemainingDownloads == 0) {
        mScene->context()->mainStrand->post(std::tr1::bind(&Entity::createMeshWork, this, md));
        return;
    }

    for(Meshdata::TextureList::const_iterator it = md->textures.begin(); it != md->textures.end(); it++) {
      String texURI = mURIString.substr(0, mURIString.rfind("/")+1) + (*it);

        ResourceDownloadTask *dl = new ResourceDownloadTask(
            Transfer::URI(texURI), getScene()->transferPool(),
            mProxy->priority,
           std::tr1::bind(&Entity::downloadFinished, this, _1, _2, md));
        (*dl)();
    }
}

void Entity::onSetScale (ProxyObjectPtr proxy, Vector3f const& scale )
{
    mSceneNode->setScale ( toOgre ( scale ) );
}

void Entity::onSetPhysical (ProxyObjectPtr proxy, PhysicalParameters const& pp )
{

}

}
}

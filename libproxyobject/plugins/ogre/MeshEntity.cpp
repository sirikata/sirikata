/*  Sirikata Graphical Object Host
 *  MeshEntity.cpp
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
#include "MeshEntity.hpp"
#include <sirikata/core/util/AtomicTypes.hpp>
#include "OgreHeaders.hpp"
#include <OgreMeshManager.h>
#include <OgreResourceGroupManager.h>
#include <OgreSubEntity.h>
#include <OgreEntity.h>
#include "resourceManager/GraphicsResourceManager.hpp"
#include "WebView.hpp"
#include <sirikata/core/util/Sha256.hpp>
#include <sirikata/core/transfer/TransferManager.hpp>
#include <sirikata/core/transfer/TransferPool.hpp>
#include <stdio.h>
#include "meruCompat/SequentialWorkQueue.hpp"
#include "resourceManager/GraphicsResourceManager.hpp"
#include "resourceManager/ResourceDownloadTask.hpp"
#include "Lights.hpp"
#include <boost/lexical_cast.hpp>


using namespace std;
using namespace Sirikata::Transfer;
using namespace Meru;

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
	const MeshEntity::TextureBindingsMap &mFrom;
public:
	ShouldReplaceTexture(const MeshEntity::TextureBindingsMap &from)
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

WebView *MeshEntity::getWebView(int whichSubEnt) {
	Ogre::Entity *ent = getOgreEntity();
	if (!ent) {
		return NULL;
	}
	if (whichSubEnt >= (int)(ent->getNumSubEntities()) || whichSubEnt < 0) {
		return NULL;
	}
	ReplacedMaterialMap::iterator iter = mReplacedMaterials.find(whichSubEnt);
	if (iter != mReplacedMaterials.end()) {
		WebView *wv = WebViewManager::getSingleton().getWebView(iter->second.first);
		return wv;
	}
	return NULL;
}

void MeshEntity::fixTextures() {
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

void MeshEntity::bindTexture(const std::string &textureName, const SpaceObjectReference &objId) {
	mTextureBindings[textureName] = objId.toString();
	fixTextures();
}
void MeshEntity::unbindTexture(const std::string &textureName) {
	TextureBindingsMap::iterator iter = mTextureBindings.find(textureName);
	if (iter != mTextureBindings.end()) {
		mTextureBindings.erase(iter);
	}
	fixTextures();
}

void MeshEntity::loadMesh(const String& meshname)
{
    Ogre::Entity * oldMeshObj=getOgreEntity();
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
    fixTextures();
}

void MeshEntity::unloadMesh() {
    Ogre::Entity * meshObj=getOgreEntity();
    //init(getScene()->getSceneManager()->createEntity(ogreMovableName(), Ogre::SceneManager::PT_CUBE));
    init(NULL);
    if (meshObj) {
        getScene()->getSceneManager()->destroyEntity(meshObj);
    }
    mReplacedMaterials.clear();
}

void MeshEntity::setSelected(bool selected) {
    Entity::setSelected(selected);
    WebView *webView = WebViewManager::getSingleton().getWebView(id().toString());
    if (selected) {
        WebViewManager::getSingleton().focusWebView(webView);
    } else if (WebViewManager::getSingleton().getFocusedWebView() == webView) {
        WebViewManager::getSingleton().deFocusAllWebViews();
    }
}

void MeshEntity::MeshDownloaded(std::tr1::shared_ptr<ChunkRequest>request, std::tr1::shared_ptr<DenseData> response)
{
    String fn = request->getURI().filename();
    if (fn.rfind(".dae") == fn.size() - 4) {
        ProxyObject *obj = mProxy.get();
        ProxyMeshObject *meshProxy = dynamic_cast<ProxyMeshObject *>(obj);
        if (meshProxy) {
            meshProxy->meshDownloaded(request, response);
        }
    }
}

void MeshEntity::downloadMeshFile(URI const& uri)
{
    ResourceDownloadTask *dl = new ResourceDownloadTask(NULL, uri, NULL, std::tr1::bind(&MeshEntity::MeshDownloaded,
            this, std::tr1::placeholders::_1, std::tr1::placeholders::_2));
    (*dl)();
}


/////////////////////////////////////////////////////////////////////
// overrides from MeshListener
// MCB: integrate these with the MeshObject model class

void MeshEntity::onSetMesh (ProxyObjectPtr proxy, URI const& meshFile )
{
    downloadMeshFile(meshFile);

    // MCB: responsibility to load model meshes must move to MeshObject plugin
    /// hack to support collada mesh -- eventually this should be smarter
    String fn = meshFile.filename();
    bool is_collada=false;
    if (fn.rfind(".dae")==fn.size()-4) is_collada=true;
    if (is_collada) {
        Meru::GraphicsResourceManager* grm = Meru::GraphicsResourceManager::getSingletonPtr ();
        Meru::SharedResourcePtr newModelPtr = grm->getResourceAsset ( meshFile, Meru::GraphicsResource::MODEL );
        mResource->setMeshResource ( newModelPtr );
    }
    else {
        Meru::GraphicsResourceManager* grm = Meru::GraphicsResourceManager::getSingletonPtr ();
        Meru::SharedResourcePtr newMeshPtr = grm->getResourceAsset ( meshFile, Meru::GraphicsResource::MESH );
        mResource->setMeshResource ( newMeshPtr );
    }
}

Vector3f fixUp(int up, Vector3f v) {
    return v;
    if (up==3) return Vector3f(v[0],v[2], -v[1]);
    else if (up==2) return v;
    std::cerr << "ERROR: X up? You gotta be frakkin' kiddin'\n";
    assert(false);
}

bool MeshEntity::createMeshWork(const Meshdata& md) {
    createMesh(md);
    return true;
}


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
        SHA256 sha = SHA256::computeDigest(md.uri);    /// rest of system uses hash
        String hash = sha.convertToHexString();
        bool useSharedBuffer = true;
        size_t totalVertexCount=0;
        int up = md.up_axis;
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
            Ogre::SubMesh *osubmesh = mesh->createSubMesh(submesh.name);
            AxisAlignedBox ogresubmeshaabb(Graphics::toOgre(geoinst.aabb.min()),
                                           Graphics::toOgre(geoinst.aabb.max()));
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
                std::string matname = md.textures.size() > 0 ?
                    hash + "_texture_" + md.textures[0] :
                    "baseogremat";
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
                    for (int i=0;i<vertcount; ++i) {
                        Vector3f v = fixUp(up, submesh.positions[i]);
                        Vector4f v_xform = pos_xform * Vector4f(v[0], v[1], v[2], 1.f);
                        v = Vector3f(v_xform[0], v_xform[1], v_xform[2]);
                        memcpy(pData,&v.x,sizeof(float));
                        memcpy(pData+sizeof(float),&v.y,sizeof(float));
                        memcpy(pData+2*sizeof(float),&v.z,sizeof(float));
                        pData+=VertexElement::getTypeSize(VET_FLOAT3);
                        if (submesh.normals.size()==submesh.positions.size()) {
                            Vector3f normal = fixUp(up, submesh.normals[i]);
                            normal = (normal_xform * normal).normal();
                            memcpy(pData,&normal.x,sizeof(float));
                            memcpy(pData+sizeof(float),&normal.y,sizeof(float));
                            memcpy(pData+2*sizeof(float),&normal.z,sizeof(float));
                            pData+=VertexElement::getTypeSize(VET_FLOAT3);
                        }
                        if (submesh.tangents.size()==submesh.positions.size()) {
                            Vector3f tangent = fixUp(up, submesh.tangents[i]);
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
                            
                            memcpy(pData,&submesh.texUVs[tc].uvs[i*stride],sizeof(float)*stride);
                            pData += VertexElement::getTypeSize(vet);
                        }
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

void MeshEntity::createMesh(const Meshdata& md) {
    SHA256 sha = SHA256::computeDigest(md.uri);    /// rest of system uses hash
    String hash = sha.convertToHexString();


    Ogre::MaterialManager& matm = Ogre::MaterialManager::getSingleton();
    Ogre::MaterialPtr base_mat = matm.getByName("baseogremat");
    for(Meshdata::TextureList::const_iterator tex_it = md.textures.begin(); tex_it != md.textures.end(); tex_it++) {
        std::string matname = hash + "_texture_" + *tex_it;
        Ogre::MaterialPtr mat = base_mat->clone(matname);
        String texURI = mURI.substr(0, mURI.rfind("/")+1) + *tex_it;
        mat->getTechnique(0)->getPass(0)->createTextureUnitState("Cache/" + mTextureFingerprints[texURI], 0);
    }
    Ogre::MeshManager& mm = Ogre::MeshManager::getSingleton();
    if (1) {
        /// FIXME: set bounds, bounding radius here
        Ogre::ManualResourceLoader *reload;
        Ogre::MeshPtr mo (mm.createManual(hash,Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,(reload=OGRE_NEW MeshdataManualLoader(md))));
        reload->prepareResource(&*mo);
        reload->loadResource(&*mo);
    }else {
        int up = md.up_axis;
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
                    Vector3f v = fixUp(up, submesh.positions[j]);
                    Vector4f v_xform = pos_xform * Vector4f(v[0], v[1], v[2], 1.f);
                    v = Vector3f(v_xform[0], v_xform[1], v_xform[2]);
                    mo.position(v[0], v[1], v[2]);
                    std::cerr<<"Mo pos "<<v[0]<<","<<v[1]<<","<<v[2]<<"\n";
                    Vector3f normal = fixUp(up, submesh.normals[j]);
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
    loadMesh(hash);                     /// this is here because we removed
                                        /// mResource->loaded(true, mEpoch) in
                                        /// ModelLoadTask::doRun

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

        String lightname = hash + "_light_" + boost::lexical_cast<String>(light_idx++);
        Ogre::Light* light = constructOgreLight(getScene()->getSceneManager(), lightname, sublight);
        mLights.push_back(light);
        mSceneNode->attachObject(light);
        light->setDebugDisplayEnabled(true);
    }
}

void MeshEntity::downloadFinished(std::tr1::shared_ptr<ChunkRequest> request,
    std::tr1::shared_ptr<DenseData> response, Meshdata& md) {

    mTextureFingerprints[request->getURI().toString()] = request->getIdentifier();

    mRemainingDownloads--;
    if (mRemainingDownloads == 0)
        Meru::SequentialWorkQueue::getSingleton().queueWork(std::tr1::bind(&MeshEntity::createMeshWork, this, md));
}

void MeshEntity::onMeshParsed(ProxyObjectPtr proxy, String const& uri, Meshdata& md) {
    mURI = uri;

    mRemainingDownloads = md.textures.size();

    // Special case for no dependent downloads
    if (mRemainingDownloads == 0) {
        Meru::SequentialWorkQueue::getSingleton().queueWork(std::tr1::bind(&MeshEntity::createMeshWork, this, md));
        return;
    }

    for(Meshdata::TextureList::const_iterator it = md.textures.begin(); it != md.textures.end(); it++) {
        String texURI = uri.substr(0, uri.rfind("/")+1) + *it;

        ResourceDownloadTask *dl = new ResourceDownloadTask(NULL, Transfer::URI(texURI), NULL,
           std::tr1::bind(&MeshEntity::downloadFinished, this, _1, _2, md));
        (*dl)();
    }
}

void MeshEntity::onSetScale (ProxyObjectPtr proxy, Vector3f const& scale )
{
    mSceneNode->setScale ( toOgre ( scale ) );
}

void MeshEntity::onSetPhysical (ProxyObjectPtr proxy, PhysicalParameters const& pp )
{

}

} // namespace Graphics
} // namespace Sirikata

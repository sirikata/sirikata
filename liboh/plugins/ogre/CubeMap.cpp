/*  Sirikata liboh -- Ogre Graphics Plugin
 *  CubeMap.cpp
 *
 *  Copyright (c) 2009, Daniel Reiter Horn
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
#include "OgreSystem.hpp"
#include "CameraEntity.hpp"
#include <Ogre.h>
#include "CubeMap.hpp"

class xTest :public Ogre::RenderTargetListener{
public:
void preRenderTargetUpdate(const Ogre::RenderTargetEvent&evt) {
    printf ("Cool %x\n",(int)(size_t)this);
}
};
namespace Sirikata { namespace Graphics {
namespace{
void faceCameraIndex(Ogre::Camera*cam, int renderTargetIndex) {
    cam->setOrientation(Ogre::Quaternion::IDENTITY);
    switch (renderTargetIndex) {
      case 0:
        
        cam->yaw(-Ogre::Radian(Ogre::Math::PI/2));
        break;
      case 1:
        cam->yaw(Ogre::Radian(Ogre::Math::PI/2));
        break;
      case 2:
        cam->pitch(Ogre::Radian(Ogre::Math::PI/2));
        break;
      case 3:
        cam->pitch(-Ogre::Radian(Ogre::Math::PI/2));
        break;
      case 5:
         cam->yaw( Ogre::Radian( Ogre::Math::PI ) );
         break;
      case 4:
        break;
    }
}
}
String CubeMap::createMaterialString(const String&textureName) {
    return "material "+textureName+"Mat\n"
        "{\n"
        "\ttechnique\n"
        "\t{\n"
        "\t\tpass\n"
        "\t\t{\n"
        "\t\t\ttexture_unit\n"
        "\t\t\t{\n"
        "\t\t\t\tcubic_texture "+textureName+" separateUV\n"
        "\t\t\t}\n"
        "\t\t}\n"    
        "\t}\n"    
        "}\n";
}
void CubeMap::swapBuffers() {
    for (int i=0;i<6;++i) {
        Ogre::TexturePtr tmp=mBackbuffer[i];
        mBackbuffer[i]=mFrontbuffer[i];
        mFrontbuffer[i]=tmp;
    }
}
CubeMap::CubeMap(OgreSystem*parent,const std::vector<String>&cubeMapTexture, int size, const std::vector<Vector3f>&cameraDelta){
    assert(cubeMapTexture.size()==1);
    
    mFaceCounter=mMapCounter=0;
    mFrontbufferCloser=true;
    mAlpha=0;
    mParent=parent;
    char cameraIdentifier;
    int num_mipmaps=0;
    for (int tsize=size;tsize;tsize/=2) {
        num_mipmaps++;
    }
    for (size_t i=0;i<cubeMapTexture.size();++i) {
        Ogre::ResourcePtr tmp=Ogre::TextureManager::getSingleton().getByName(cubeMapTexture[i]);
        if ( tmp.get()) {
            mCubeMapTextures.push_back(tmp);
        }else {            
            mCubeMapTextures.push_back(Ogre::TextureManager::getSingleton()
                                  .createManual(cubeMapTexture[i],
                                                Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                                                Ogre::TEX_TYPE_CUBE_MAP,
                                                size,
                                                size,
                                                1,
                                                0,
                                                Ogre::PF_A8R8G8B8,
                                                Ogre::TU_RENDERTARGET|Ogre::TU_AUTOMIPMAP));
        }
    }
    for (int j=0;j<2;++j) {
        for (int i=0;i<6;++i){
            (j?mBackbuffer:mFrontbuffer)[i] = Ogre::TextureManager::getSingleton()
                .createManual(UUID::random().toString(),
                              Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                              Ogre::TEX_TYPE_2D,
                              size,
                              size,
                              1,
                              0,
                              Ogre::PF_A8R8G8B8,
                              Ogre::TU_RENDERTARGET);
        }
    }
    mCubeMapScene=Ogre::Root::getSingleton().createSceneManager("DefaultSceneManager");
    for (int i=0;i<6;++i) {
        mCubeMapSceneCamera[i]=mCubeMapScene->createCamera(mBackbuffer[i]->getName());
        mCubeMapSceneCamera[i]->setPosition(0,0,0);//FIXME we want this
        mCubeMapScene->getRootSceneNode()->attachObject( mCubeMapSceneCamera[i]);
        //mCubeMapSceneCamera[i]=mParent->getSceneManager()->createCamera(mBackbuffer->getName()+labele);
        //mCubeMapSceneCamera[i]->setPosition(0,4594,0);

        faceCameraIndex(mCubeMapSceneCamera[i],i);
        mCubeMapSceneCamera[i]->setNearClipDistance(0.1f);
        mCubeMapSceneCamera[i]->setAspectRatio(1);
        mCubeMapSceneCamera[i]->setFOVy(Ogre::Radian(Ogre::Math::PI/2));

    
	for (int j=0;j<2;++j) {
	  Ogre::MaterialPtr mat=mMaterials[i]=Ogre::MaterialManager::getSingleton().create((j?mBackbuffer:mFrontbuffer)[i]->getName()+"Mat",Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
	  mat->setCullingMode(Ogre::CULL_NONE);
	  Ogre::Technique* tech=mat->getTechnique(0);
	  Ogre::Pass*pass=tech->getPass(0);
/*
      pass->setDepthCheckEnabled(false);
      pass->setReceiveShadows(false);
      pass->setLightingEnabled(true);
      if (j==0) {
          //frontbuffer, turn on alpha!
          pass->setSceneBlending(Ogre::SBF_SOURCE_ALPHA,
                                Ogre::SBF_ONE_MINUS_SOURCE_ALPHA);
          assert(mat->isTransparent());
          pass->setAmbient(Ogre::ColourValue(0.,1.,0.,0));
          pass->setDiffuse(Ogre::ColourValue(0.,0.,0.,0));
          pass->setSpecular(Ogre::ColourValue(0.,0.,0.,0));
      }else {
          pass->setAmbient(Ogre::ColourValue(.1,0.,0.,1.));
          pass->setDiffuse(Ogre::ColourValue(0.,0.,0.,0));
          pass->setSpecular(Ogre::ColourValue(0.,0.,0.,0));
      }
*/
	  Ogre::TextureUnitState*tus=pass->createTextureUnitState();
	  tus->setTextureName((j?mBackbuffer:mFrontbuffer)[i]->getName());
	  tus->setTextureCoordSet(0);
      if (j){
          Ogre::TextureUnitState*tus=pass->createTextureUnitState();
          tus->setTextureName(mFrontbuffer[i]->getName());
          tus->setTextureCoordSet(0);
          tus->setColourOperationEx(Ogre::LBX_BLEND_MANUAL,
                                    Ogre::LBS_TEXTURE,
                                    Ogre::LBS_CURRENT,
                                    Ogre::ColourValue::White,Ogre::ColourValue::White,
                                    .25);
          tus->setColourOpMultipassFallback(Ogre::SBF_SOURCE_ALPHA,Ogre::SBF_ONE_MINUS_SOURCE_ALPHA);
      }      
	  bool reverse=true;//defeat ogre reflection vector bug
	  Ogre::Vector3 offset;
	  Ogre::Vector3 up;
	  switch (i){
          case 0:
            offset=Ogre::Vector3(-1,0,0);
            up=Ogre::Vector3(0,1,0);
            break;
          case 1:
            offset=Ogre::Vector3(1,0,0);
            up=Ogre::Vector3(0,1,0);
            break;
          case 2:
            offset=Ogre::Vector3(0,-1,0);
            up=Ogre::Vector3(0,0,1);
            break;
          case 3:
            offset=Ogre::Vector3(0,1,0);
            up=Ogre::Vector3(0,0,1);
            break;
          case 4:
            offset=Ogre::Vector3(0,0,reverse?-1:1);
            up=Ogre::Vector3(0,1,0);
            break;
          case 5:
            offset=Ogre::Vector3(0,0,reverse?1:-1);
            up=Ogre::Vector3(0,1,0);
            break;
	  }
	  float sizeScale=(j?2.0f:1.0f);
	  Ogre::MeshPtr msh=Ogre::MeshManager::getSingleton().createPlane((j?mBackbuffer:mFrontbuffer)[i]->getName()+"Plane",
									  Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
									  Ogre::Plane((reverse&&(i==1||i==0||i==4||i==5))?-offset:offset,0),
									  sizeScale,sizeScale,1,1,false,1,1.0,((reverse&&(i==3||i==2))?-1.0:1.0),up);
	  Ogre::Entity* ent=mCubeMapScene->createEntity((j?mBackbuffer:mFrontbuffer)[i]->getName()+"Entity",
							(j?mBackbuffer:mFrontbuffer)[i]->getName()+"Plane");
	  ent->setMaterialName(mat->getName()); 
      if (j)
          mCubeMapScene->getRootSceneNode()->createChildSceneNode(-offset*0.5f*sizeScale)->attachObject(ent);
	}
    }
    mCameraDelta=cameraDelta;
    mCamera=mParent->getSceneManager()->createCamera(mBackbuffer[0]->getName()+"Camera");
    //mParent->getSceneManager()->getRootSceneNode()->attachObject( mCamera);
    mCamera->setNearClipDistance(0.1f);
    mCamera->setAspectRatio(1);
    mCamera->setFOVy(Ogre::Radian(Ogre::Math::PI/2));

    for (unsigned int i=0;i<6;++i)
    {
        mFaces[i].mParent=this;
        
        Ogre::RenderTarget *renderTarget =mBackbuffer[i]->getBuffer(0)->getRenderTarget(); //mBackbuffer->getBuffer(i)->getRenderTarget();
        renderTarget->addListener(&mFaces[i]);
        renderTarget =mFrontbuffer[i]->getBuffer(0)->getRenderTarget(); //mBackbuffer->getBuffer(i)->getRenderTarget();
        renderTarget->addListener(&mFaces[i]);
    } 
}

CubeMap::BlendProgress CubeMap::updateBlendState(const Ogre::FrameEvent&evt) {
    CubeMap::BlendProgress retval=DOING_BLENDING;
    if (mFrontbufferCloser) {
        mAlpha+=.03125;//FIXME should relate to evt
    }else {
        mAlpha-=.03125;
    }
    if (mAlpha<0) {
        mFrontbufferCloser=true;
        mAlpha=0;
        retval= DONE_BLENDING;
    }
    if(mAlpha>1) {
        mFrontbufferCloser=false;
        mAlpha=1;
        retval= DONE_BLENDING;
    }
    for (int i=0;i<6;++i) {
        mMaterials[i]->getTechnique(0)->getPass(0)->getTextureUnitState(1)->
            setColourOperationEx(Ogre::LBX_BLEND_MANUAL,
                                 Ogre::LBS_TEXTURE,
                                 Ogre::LBS_CURRENT,
                                 Ogre::ColourValue::White,Ogre::ColourValue::White,
                                 1.0-mAlpha);
        
    }
    return retval;
}


bool CubeMap::frameEnded(const Ogre::FrameEvent&evt) {
    for (int i=0;i<6;++i) {
        //mCubeMapSceneCamera[i]->setPosition(toOgre(mParent->getPrimaryCamera()->getOgrePosition()+Vector3d(mCameraDelta[0]),mParent->getOffset()));        
    }

    if (mFaceCounter>0&&mFaceCounter<7) {
        mBackbuffer[mFaceCounter-1]->getBuffer(0)->getRenderTarget()->removeAllViewports();        
    }
    if (mFaceCounter<6) {
        Ogre::Viewport *viewport = mBackbuffer[mFaceCounter]->getBuffer(0)->getRenderTarget()->addViewport( mCamera );        
        viewport->setOverlaysEnabled(false);
        viewport->setClearEveryFrame( true );
        viewport->setBackgroundColour( Ogre::ColourValue(0,1,1,1) );
    }

    if (mFaceCounter==7) {

        
        for (int i=0;i<6;++i) {
            Ogre::Viewport *viewport = mCubeMapTextures[mMapCounter]->getBuffer(i)->getRenderTarget()->addViewport( mCubeMapSceneCamera[i] );
            viewport->setOverlaysEnabled(false);
            viewport->setClearEveryFrame( true );
            viewport->setBackgroundColour( Ogre::ColourValue(1,0,0,1) );
        }
    }
    if (mFaceCounter!=8||updateBlendState(evt)==DONE_BLENDING) {
      ++mFaceCounter;
    }
    if (mFaceCounter==9) {
        mCamera->setPosition(toOgre(mParent->getPrimaryCamera()->getOgrePosition()+Vector3d(mCameraDelta[mMapCounter]),mParent->getOffset()));
        for (int i=0;i<6;++i) {
            mCubeMapTextures[mMapCounter]->getBuffer(i)->getRenderTarget()->removeAllViewports();
        }
        mFaceCounter=0;
        mMapCounter++;
        if ((size_t)mMapCounter==mCubeMapTextures.size()) {
            mMapCounter=0;
	    swapBuffers();
        }
    }  
    return true;
}
void CubeMap::CubeMapFace::preRenderTargetUpdate(const Ogre::RenderTargetEvent&evt) {
    mParent->preRenderTargetUpdate(mParent->mCamera,this-&mParent->mFaces[0],evt);
}
void CubeMap::preRenderTargetUpdate(Ogre::Camera*cam, int renderTargetIndex,const Ogre::RenderTargetEvent&evt) {
    faceCameraIndex(cam,renderTargetIndex);
}
CubeMap::~CubeMap() {
    for (unsigned int i=0;i<6;++i)
    {
        Ogre::RenderTarget *renderTarget = mBackbuffer[i]->getBuffer(0)->getRenderTarget();
        renderTarget->removeListener(&mFaces[i]);
        renderTarget->removeAllViewports();
    }
    for (int i=0;i<6;++i) {
        Ogre::MaterialManager::getSingleton().remove(mBackbuffer[i]->getName()+"Mat");
        mCubeMapScene->destroyCamera(mCubeMapSceneCamera[i]);
        Ogre::TextureManager::getSingleton().remove(mBackbuffer[i]->getName());
    }
    Ogre::Root::getSingleton().destroySceneManager(mCubeMapScene);
}
} }

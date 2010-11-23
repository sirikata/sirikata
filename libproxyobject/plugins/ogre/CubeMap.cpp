/*  Sirikata libproxyobject -- Ogre Graphics Plugin
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
#include <sirikata/proxyobject/Platform.hpp>
#include "OgreSystem.hpp"
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
        mBackbuffer[i]=mState[mMapCounter].mFrontbuffer[i];
        mState[mMapCounter].mFrontbuffer[i]=tmp;
    }
}
CubeMap::CubeMap(OgreSystem*parent,const std::vector<String>&cubeMapTexture, int size, const std::vector<Vector3f>&cameraDelta, const std::vector<float>&cameraNearPlanes){
    bool valid=true;
    mFaceCounter=mMapCounter=0;
    mFrontbufferCloser=true;
    mAlpha=0;
    mParent=parent;
    int num_mipmaps=0;
    for (int tsize=size;tsize;tsize/=2) {
        num_mipmaps++;
    }
    for (size_t i=0;i<cubeMapTexture.size();++i) {
        Ogre::ResourcePtr tmp=Ogre::TextureManager::getSingleton().getByName(cubeMapTexture[i]);
        if ( tmp.get()) {
            Ogre::TextureManager::getSingleton().remove(tmp);
        }
        mState.push_back(PerCubeMapState());
        try {
            mState.back().mCubeMapTexture=Ogre::TextureManager::getSingleton()
                .createManual(cubeMapTexture[i],
                              Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                              Ogre::TEX_TYPE_CUBE_MAP,
                              size,
                              size,
                              1,
                              0,
                              Ogre::PF_A8R8G8B8,
                              Ogre::TU_RENDERTARGET|Ogre::TU_AUTOMIPMAP);
        }catch (...) {
            if (valid) {
                for (size_t j=0;j<i;++j) {
                    mParent->getSceneManager()->destroyCamera(mState[j].mCamera);
                    Ogre::TextureManager::getSingleton().remove(mState[j].mCubeMapTexture->getName());
                }
            }
            valid=false;
            SILOG(ogre,warning,"Cannot allocate cube maps sized "<<size<<'x'<<size<<" with automimap");
            throw std::bad_alloc();//cannot handle so many cubemaps
        }
        mState.back().mCameraDelta=cameraDelta[i];
        mState.back().mCamera=mParent->getSceneManager()->createCamera(UUID::random().toString()+"CubeMapCamera");
        mState.back().mLastRenderedPosition=Ogre::Vector3(0,0,0);
        mState.back().mLastActualPosition=Ogre::Vector3(0,0,0);
        mState[mMapCounter].mFirstCameraPosition=Ogre::Vector3(0,0,0);
        //mParent->getSceneManager()->getRootSceneNode()->attachObject( mCamera);
        mState.back().mCamera->setNearClipDistance(cameraNearPlanes[i]);
        mState.back().mCamera->setAspectRatio(1);
        mState.back().mCamera->setFOVy(Ogre::Radian(Ogre::Math::PI/2));

    }
    std::vector<String> made_so_far;
    if (valid) try {
        for (size_t j=0;j<=cubeMapTexture.size();++j) {
            for (int i=0;i<6;++i){
                String cur=UUID::random().toString();
                (j==0?mBackbuffer:mState[j-1].mFrontbuffer)[i] = Ogre::TextureManager::getSingleton()
                    .createManual(cur,
                                  Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                                  Ogre::TEX_TYPE_2D,
                                  size,
                                  size,
                                  1,
                                  0,
                                  Ogre::PF_A8R8G8B8,
                                  Ogre::TU_RENDERTARGET);
                made_so_far.push_back(cur);
            }
        }
    }catch (...) {
        if (valid) {
            for (size_t i=0;i < made_so_far.size();++i) {
                    Ogre::TextureManager::getSingleton().remove(made_so_far[i]);
            }
            for (size_t j=0;j<mState.size();++j) {
                mParent->getSceneManager()->destroyCamera(mState[j].mCamera);
                Ogre::TextureManager::getSingleton().remove(mState[j].mCubeMapTexture->getName());
            }
        }
        valid=false;
            SILOG(ogre,warning,"Cannot allocate "<<made_so_far.size()+1<<"th renderable texture sized "<<size<<'x'<<size);
            throw std::bad_alloc();//cannot handle so many cubemaps
    }
    mCubeMapScene=Ogre::Root::getSingleton().createSceneManager("DefaultSceneManager");
    for (int i=0;i<6;++i) {
        mCubeMapSceneCamera[i]=mCubeMapScene->createCamera(mBackbuffer[i]->getName());
        mCubeMapSceneCamera[i]->setPosition(0,0,0);
        mCubeMapScene->getRootSceneNode()->attachObject( mCubeMapSceneCamera[i]);
        faceCameraIndex(mCubeMapSceneCamera[i],i);
        mCubeMapSceneCamera[i]->setNearClipDistance(0.1f);
        mCubeMapSceneCamera[i]->setAspectRatio(1);
        mCubeMapSceneCamera[i]->setFOVy(Ogre::Radian(Ogre::Math::PI/2));

        bool reverse=true;//defeat ogre reflection vector bug
        Ogre::Vector3 offset(0,0,0);//initialize to prevent gcc warning
        Ogre::Vector3 up(0,1,0);
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

        Ogre::MaterialPtr mat=mMaterials[i]=Ogre::MaterialManager::getSingleton()
            .create(mBackbuffer[i]->getName()+"Mat",
                    Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
        mat->setCullingMode(Ogre::CULL_NONE);
        Ogre::Technique* tech=mat->getTechnique(0);
        Ogre::Pass*pass=tech->getPass(0);
        Ogre::TextureUnitState*tus=pass->createTextureUnitState();
        tus->setTextureName(mBackbuffer[i]->getName());
        tus->setTextureCoordSet(0);
        tus=pass->createTextureUnitState();
        tus->setTextureName(mState[0].mFrontbuffer[i]->getName());
        tus->setTextureCoordSet(0);
        tus->setColourOperationEx(Ogre::LBX_BLEND_MANUAL,
                                  Ogre::LBS_TEXTURE,
                                  Ogre::LBS_CURRENT,
                                  Ogre::ColourValue::White,Ogre::ColourValue::White,
                                  .25);
        tus->setColourOpMultipassFallback(Ogre::SBF_SOURCE_ALPHA,Ogre::SBF_ONE_MINUS_SOURCE_ALPHA);

        float sizeScale=1.0f;
        Ogre::MeshPtr msh=Ogre::MeshManager::getSingleton().createPlane(mBackbuffer[i]->getName()+"Plane",
                                                                        Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                                                                        Ogre::Plane((reverse&&(i==1||i==0||i==4||i==5))?-offset:offset,0),
                                                                        sizeScale,sizeScale,1,1,false,1,1.0,((reverse&&(i==3||i==2))?-1.0:1.0),up);
        Ogre::Entity* ent=mCubeMapScene->createEntity(mBackbuffer[i]->getName()+"Entity",
                                                      mBackbuffer[i]->getName()+"Plane");
        ent->setMaterialName(mBackbuffer[i]->getName()+"Mat");
        mCubeMapScene->getRootSceneNode()->createChildSceneNode(-offset*0.5f*sizeScale)->attachObject(ent);
        mFaces[i].mParent=this;

        Ogre::RenderTarget *renderTarget =mBackbuffer[i]->getBuffer(0)->getRenderTarget();
        renderTarget->addListener(&mFaces[i]);
        for (size_t j=0;j<mState.size();++j) {
            renderTarget =mState[j].mFrontbuffer[i]->getBuffer(0)->getRenderTarget();
            renderTarget->addListener(&mFaces[i]);
        }
    }
}
float clamp01(float input) {
    if (input>1) return 1;
    if (input>0) return input;
    return 0.0;
}
CubeMap::BlendProgress CubeMap::updateBlendState(const Ogre::FrameEvent&evt) {
    CubeMap::BlendProgress retval=DOING_BLENDING;
    if (mFrontbufferCloser) {
        mAlpha+=.03125;//FIXME should relate to evt
    }else {
        mAlpha-=.03125;
    }
    for (int i=0;i<6;++i) {
        mMaterials[i]->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName(mBackbuffer[i]->getName());
        mMaterials[i]->getTechnique(0)->getPass(0)->getTextureUnitState(1)->setTextureName(mState[mMapCounter].mFrontbuffer[i]->getName());
        mMaterials[i]->getTechnique(0)->getPass(0)->getTextureUnitState(1)->
            setColourOperationEx(Ogre::LBX_BLEND_MANUAL,
                                 Ogre::LBS_TEXTURE,
                                 Ogre::LBS_CURRENT,
                                 Ogre::ColourValue::White,Ogre::ColourValue::White,
                                 clamp01(mFrontbufferCloser?1.0-mAlpha:mAlpha));

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
    return retval;
}
bool CubeMap::tooSmall(Ogre::Vector3 delta) {
    if (delta.x<0) delta.x=-delta.x;
    if (delta.y<0) delta.y=-delta.y;
    if (delta.z<0) delta.z=-delta.z;
    if (delta.x<.03125&&delta.y<.03125&&delta.z<.03125) {
        return true;
    }
    return false;
}

bool CubeMap::frameEnded(const Ogre::FrameEvent&evt) {

    if (mFaceCounter==0) {
        Ogre::Vector3 curCamera=toOgre(mParent->getPrimaryCamera()->following()->getOgrePosition(),mParent->getOffset());
        Ogre::Vector3 delta=curCamera-mState[mMapCounter].mLastActualPosition;
        if (mState[mMapCounter].mLastActualPosition==Ogre::Vector3(0,0,0)) {
            mState[mMapCounter].mFirstCameraPosition=curCamera;
        }
        mState[mMapCounter].mLastActualPosition=curCamera;
        curCamera+=delta;
        if (tooSmall(curCamera-mState[mMapCounter].mLastRenderedPosition)&&!(curCamera==mState[mMapCounter].mFirstCameraPosition)) {
            mFaceCounter=9;//abort abort: this cubemap is too similar!
        }else {
            mState[mMapCounter].mLastRenderedPosition=curCamera;
        }
        mState[mMapCounter].mCamera->setPosition(toOgre(mParent->getPrimaryCamera()->following()->getOgrePosition()+Vector3d(mState[mMapCounter].mCameraDelta),mParent->getOffset()));
    }
    if (mFaceCounter>0&&mFaceCounter<7) {
        mBackbuffer[mFaceCounter-1]->getBuffer(0)->getRenderTarget()->removeAllViewports();
    }
    if (mFaceCounter<6) {
        Ogre::Viewport *viewport = mBackbuffer[mFaceCounter]->getBuffer(0)->getRenderTarget()->addViewport( mState[mMapCounter].mCamera );
        viewport->setOverlaysEnabled(false);
        viewport->setClearEveryFrame( true );
        viewport->setBackgroundColour( Ogre::ColourValue(0,0,0,0) );
    }

    BlendProgress progress=DOING_BLENDING;
    if (mFaceCounter==7||mFaceCounter==8) {

        progress=updateBlendState(evt);
    }
    if (mFaceCounter==7) {
        for (int i=0;i<6;++i) {
            Ogre::Viewport *viewport = mState[mMapCounter].mCubeMapTexture->getBuffer(i)->getRenderTarget()->addViewport( mCubeMapSceneCamera[i] );
            viewport->setOverlaysEnabled(false);
            viewport->setClearEveryFrame( true );
            viewport->setBackgroundColour( Ogre::ColourValue(1,0,0,1) );
        }
    }
    if (mFaceCounter!=8||progress==DONE_BLENDING) {
      ++mFaceCounter;
    }

    if (mFaceCounter>=9) {
        for (int i=0;i<6;++i) {
            mState[mMapCounter].mCubeMapTexture->getBuffer(i)->getRenderTarget()->removeAllViewports();
        }
	    swapBuffers();
        mFaceCounter=0;
        mMapCounter++;
        if ((size_t)mMapCounter==mState.size()) {
            mMapCounter=0;
        }
    }
    return true;
}
void CubeMap::CubeMapFace::preRenderTargetUpdate(const Ogre::RenderTargetEvent&evt) {
    mParent->preRenderTargetUpdate(NULL,this-&mParent->mFaces[0],evt);
}
void CubeMap::preRenderTargetUpdate(Ogre::Camera*cam, int renderTargetIndex,const Ogre::RenderTargetEvent&evt) {
    faceCameraIndex(mState[mMapCounter].mCamera,renderTargetIndex);
}
CubeMap::~CubeMap() {
    for (size_t j=0;j<mState.size();++j) {
        mParent->getSceneManager()->destroyCamera(mState[j].mCamera);
    }
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

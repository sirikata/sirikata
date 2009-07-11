/*  Sirikata liboh -- Ogre Graphics Plugin
 *  CubeMap.hpp
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
#ifndef _SIRIKATA_CUBE_MAP_HPP_
#define _SIRIKATA_CUBE_MAP_HPP_
namespace Sirikata {
namespace Graphics {


class CubeMap {//:public Ogre::FrameListener{
    Ogre::SceneManager*mCubeMapScene;
    Ogre::Camera*mCubeMapSceneCamera[6];
    OgreSystem*mParent;
    Ogre::Camera*mCamera;
    float mAlpha;
    bool mFrontbufferCloser;
    std::vector<Ogre::TexturePtr> mCubeMapTextures;
    Ogre::TexturePtr mBackbuffer[6];
    Ogre::MaterialPtr mMaterials[6];
    Ogre::TexturePtr mFrontbuffer[6];
    void swapBuffers();
    class CubeMapFace:public Ogre::RenderTargetListener{
    public:
        CubeMap*mParent;
        void preRenderTargetUpdate(const Ogre::RenderTargetEvent& evt);        
    };
    friend class CubeMapFace;
    CubeMapFace mFaces[6];
    std::vector<Vector3f> mCameraDelta;
    int mFaceCounter;
    int mMapCounter;
    String createMaterialString(const String&materialName);
    enum BlendProgress{
      DOING_BLENDING,
      DONE_BLENDING
    };
    BlendProgress updateBlendState(const Ogre::FrameEvent&evt);
public:
    CubeMap(OgreSystem*parent, const std::vector<String>&cubeMapTexture, int size, const std::vector<Vector3f> &mCameraDelta);
    ~CubeMap();
    bool frameEnded(const Ogre::FrameEvent&evt);
    void preRenderTargetUpdate(Ogre::Camera*cam,int renderTargetIndex,const Ogre::RenderTargetEvent& evt);
};
}
}
#endif

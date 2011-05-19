/*  Sirikata
 *  OgreRenderer.hpp
 *
 *  Copyright (c) 2011, Stanford University
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

#ifndef _SIRIKATA_OGRE_OGRE_RENDERER_HPP_
#define _SIRIKATA_OGRE_OGRE_RENDERER_HPP_

#include <sirikata/ogre/Platform.hpp>
#include <sirikata/ogre/OgreHeaders.hpp>
#include "OgreResource.h"
#include <sirikata/mesh/Meshdata.hpp>
#include <sirikata/core/service/Context.hpp>
#include <sirikata/core/transfer/TransferMediator.hpp>

namespace Sirikata {
namespace Graphics {

class Camera;
class Entity;

/** Represents a SQLite database connection. */
class SIRIKATA_OGRE_EXPORT OgreRenderer {
public:
    OgreRenderer(Context* ctx);
    virtual ~OgreRenderer();

    Context* context() const { return mContext; }



    // Event injection for SDL created windows.
    virtual void injectWindowResized(uint32 w, uint32 h) {}

    const Vector3d& getOffset() const { return mFloatingPointOffset; }

    virtual Transfer::TransferPoolPtr transferPool() = 0;

    virtual Ogre::SceneManager* getSceneManager() = 0;
    virtual Ogre::RenderTarget* createRenderTarget(const String &name, uint32 width, uint32 height, bool automipmap, int pixelFmt) = 0;
    virtual Ogre::RenderTarget* createRenderTarget(String name,uint32 width=0, uint32 height=0) = 0;
    virtual void destroyRenderTarget(Ogre::ResourcePtr& name) = 0;
    virtual void destroyRenderTarget(const String &name) = 0;
    virtual Ogre::RenderTarget *getRenderTarget() = 0;

    // Options values
    virtual float32 nearPlane() = 0;
    virtual float32 farPlane() = 0;
    virtual float32 parallaxSteps() = 0;
    virtual int32 parallaxShadowSteps() = 0;

    ///adds the camera to the list of attached cameras, making it the primary camera if it is first to be added
    virtual void attachCamera(const String& renderTargetName, Camera*) = 0;
    ///removes the camera from the list of attached cameras.
    virtual void detachCamera(Camera*) = 0;

    typedef std::tr1::function<void(Mesh::MeshdataPtr)> ParseMeshCallback;
    /** Tries to parse a mesh. Can handle different types of meshes and tries to
     *  find the right parser using magic numbers.  If it is unable to find the
     *  right parser, returns NULL.  Otherwise, returns the parsed mesh as a
     *  Meshdata object.
     *  \param orig_uri original URI, used to construct correct relative paths
     *                  for dependent resources
     *  \param fp the fingerprint of the data, used for unique naming and passed
     *            through to the resulting mesh data
     *  \param data the contents of the
     *  \param cb callback to invoke when parsing is complete
     */
    virtual void parseMesh(const Transfer::URI& orig_uri, const Transfer::Fingerprint& fp, Transfer::DenseDataPtr data, ParseMeshCallback cb) = 0;

  protected:
    Context* mContext;

    Vector3d mFloatingPointOffset;

    typedef std::tr1::unordered_map<String,Entity*> SceneEntitiesMap;
    SceneEntitiesMap mSceneEntities;
    std::list<Entity*> mMovingEntities;

    friend class Entity; //Entity will insert/delete itself from these arrays.
    friend class Camera; //CameraEntity will insert/delete itself from the scene cameras array.
};

} // namespace Graphics
} // namespace Sirikata

#endif //_SIRIKATA_OGRE_OGRE_RENDERER_HPP_

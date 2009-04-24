/*  Meru
 *  GraphicsEntity.hpp
 *
 *  Copyright (c) 2009, Stanford University
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
#ifndef _GRAPHICS_ENTITY_HPP_
#define _GRAPHICS_ENTITY_HPP_

#include "GraphicsObject.hpp"
#include "MeruDefs.hpp"
#include "Proxy.hpp"
#include "GraphicsResource.hpp"
namespace Meru {

class GraphicsCamera;
/** Base class for entities in the graphics system.  Graphics
 *  entities are created via the current graphics system
 *  and are used by proxy objects.
 */
class GraphicsEntity : public GraphicsObject {
public:
    GraphicsEntity(WeakProxyPtr parentProxyObject);
    virtual ~GraphicsEntity();

    /** Hide this graphics entity's mesh. */
    virtual void show() = 0;

    /** Hide this graphics entity's mesh. */
    virtual void hide() = 0;

    /** Load the mesh and use it for this entity
     *  \param meshname the name (ID) of the mesh to use for this entity
     */
    virtual void loadMesh(const String& meshname) = 0;

    virtual void unloadMesh() = 0;

    virtual bool isVisible() = 0;

    /**
     * Sets the scale for the mesh
     */
    virtual void setScale(const Vector3f &scale) = 0;

    /** Animate this entity with the specified animation.
     *  \param animation_name the name of the animation to use
     */
    virtual void animate(const String& animation_name) = 0;

    /** Get a camera which tracks this entity.  This camera
     *  is owned by the entity (i.e. don't delete it) and
     *  is guaranteed to exist for the lifetime of the entity.
     */
    virtual GraphicsCamera* camera() = 0;

    virtual SharedResourcePtr getResource();
    virtual void setResource(SharedResourcePtr resourcePtr);
    virtual void setUnscaledBoundingInfo(const BoundingInfo&)=0;
    ///Returns the scaled bounding info
    const BoundingInfo& getBoundingInfo()const{
        return mBoundingInfo;
    }
protected:
    /** Signal to the parent proxy object that any adjustments that
     *  needed to be made for this frame have been made.
     */

    SharedResourcePtr mResource;
    WeakProxyPtr mParentProxyObject;
    BoundingInfo mBoundingInfo;
};

} // namespace Meru


#endif //_GRAPHICS_ENTITY_HPP_

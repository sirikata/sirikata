/*  Sirikata Object Host
 *  MeshListener.hpp
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
#ifndef _SIRIKATA_MESH_LISTENER_HPP_
#define _SIRIKATA_MESH_LISTENER_HPP_

#include "transfer/URI.hpp"

namespace Sirikata {

using Transfer::URI;

// FIX ME: this definition probably doesn't belong here
struct PhysicalParameters {
    enum PhysicalMode {
        Disabled = 0,               /// non-active, remove from physics
        Static,                 /// collisions, no dynamic movement (bullet mass==0)
        DynamicBox,                 /// fully physical -- collision & dynamics
        DynamicSphere,
        DynamicCylinder,
        Character
    };

    std::string name;
    PhysicalMode mode;
    float density;
    float friction;
    float bounce;
    float gravity;
    int colMask;
    int colMsg;
    Vector3f hull;
    PhysicalParameters() :
        mode(Disabled),
        density(0),
        friction(0),
        bounce(0),
        gravity(0),
        colMask(0),
        colMsg(0),
        hull() {
    }
};

class SIRIKATA_OH_EXPORT MeshListener
{
    public:
        virtual ~MeshListener() {}

        virtual void meshChanged ( URI const& newMesh) = 0;
        virtual void scaleChanged ( Vector3f const& newScale ) = 0;
        virtual void physicalChanged ( PhysicalParameters const& pp ) = 0;
};

} // namespace Sirikata

#endif

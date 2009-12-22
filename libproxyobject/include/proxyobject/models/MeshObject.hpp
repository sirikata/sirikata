/*  Sirikata liboh -- MeshObject Model Interface (Bridge Pattern)
 *  MeshObject.hpp
 *
 *  Copyright (c) 2009, Mark C. Barnes
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

#ifndef _SIRIKATA_MESH_OBJECT_HPP_
#define _SIRIKATA_MESH_OBJECT_HPP_

#include <oh/Platform.hpp>

// MCB: move to model plugin
//#include "transfer/URI.hpp"
//#include "oh/MeshListener.hpp"

namespace Sirikata {

namespace Transfer {
        class URI;
}

class PhysicalParameters;

using Transfer::URI;

namespace Models {

/** interface to data model for mesh objects
 *  Declares the Bridge Pattern Implementor interface for proxy mesh object Abstractions.
 *  Roles are: Abstraction :: ProxyMeshObject, Implementor :: MeshObject, ConcreteImplementor :: [see liboh/plugins/collada]
 */
class SIRIKATA_OH_EXPORT MeshObject
{
    public:
        virtual ~MeshObject () {}

        virtual void setMesh ( URI const& rhs ) = 0;
        virtual URI const& getMesh () const = 0;

        virtual void setScale ( Vector3f const& rhs ) = 0;
        virtual Vector3f const& getScale () const = 0;
        
        virtual void setPhysical ( PhysicalParameters const& rhs ) = 0;
        virtual PhysicalParameters const& getPhysical () const = 0;
    
    protected:
        MeshObject () {}
//        MeshObject ( MeshObject const& rhs );
//        MeshObject& operator = ( MeshObject const& rhs );
    
    private:
    // MCB: move data members from proxy to plugin, via this route
//        URI mMeshURI;
//        Vector3f mScale;
//        PhysicalParameters mPhysical;
    
};

} // namespace Models
} // namespace Sirikata

#endif // _SIRIKATA_MESH_OBJECT_HPP_

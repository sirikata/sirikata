/*  Sirikata libproxyobject -- Collada Models Mesh Object
 *  ColladaMeshObject.hpp
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

#ifndef _SIRIKATA_COLLADA_MESH_OBJECT_
#define _SIRIKATA_COLLADA_MESH_OBJECT_

#include <proxyobject/Platform.hpp>
#include <proxyobject/models/MeshObject.hpp>

#include <proxyobject/MeshListener.hpp> // MCB: move PhysicalParameters out of here!
#include <transfer/URI.hpp>

namespace COLLADAFW {
    
class Geometry;
class Mesh;
    
}

namespace Sirikata { namespace Models {

/////////////////////////////////////////////////////////////////////
    
class ColladaDocumentImporter;
class ColladaSystem;

class SIRIKATA_PLUGIN_EXPORT ColladaMeshObject
    : public MeshObject
{
    public:
        explicit ColladaMeshObject ( ColladaSystem& system );
        ColladaMeshObject ( ColladaMeshObject const& rhs );
        ColladaMeshObject& operator = ( ColladaMeshObject const& rhs );
        virtual ~ColladaMeshObject ();

        bool import ( ColladaDocumentImporter& importer, COLLADAFW::Geometry const& geometry );
        bool import ( ColladaDocumentImporter& importer, COLLADAFW::Mesh const& mesh );

    protected:
    
    private:
        ColladaSystem& mSystem;

        URI mMeshURI;
        Vector3f mScale;
        PhysicalParameters mPhysical;
    
    // interface from MeshObject
    public:
        virtual void setMesh ( URI const& rhs );
        virtual URI const& getMesh () const;
        
        virtual void setScale ( Vector3f const& rhs );
        virtual Vector3f const& getScale () const;
        
        virtual void setPhysical ( PhysicalParameters const& rhs );
        virtual PhysicalParameters const& getPhysical () const;
        
    protected:
    
};

} // namespace Models
} // namespace Sirikata

#endif // _SIRIKATA_COLLADA_MESH_OBJECT_

/*  Sirikata libproxyobject -- Collada Models Mesh Object
 *  ColladaMeshObject.cpp
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

#include "ColladaMeshObject.hpp"

#include "ColladaSystem.hpp"
#ifndef __APPLE__
#include "COLLADABUhash_map.h"
#endif
#include "COLLADAFWGeometry.h"
#include "COLLADAFWMesh.h"

/////////////////////////////////////////////////////////////////////

namespace Sirikata { namespace Models {
    
ColladaMeshObject::ColladaMeshObject ( ColladaSystem& system )
        :   MeshObject (),
        mSystem ( system ),
        mProxyPtr()
{

}

//ColladaMeshObject::ColladaMeshObject ( ColladaMeshObject const& rhs )
//{
//    
//}

//ColladaMeshObject::ColladaMeshObject& operator = ( ColladaMeshObject const& rhs )
//{
//    
//}

ColladaMeshObject::~ColladaMeshObject ()
{

}

/////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////
// MCB: Move this method to a new class called ColladaGeometry for consistency

bool ColladaMeshObject::import ( ColladaDocumentImporter& importer, COLLADAFW::Geometry const& geometry )
{
    assert((std::cout << "MCB: ColladaMeshObject::import(COLLADAFW::Geometry) entered" << std::endl,true));
    
    bool ok = false;
    
    if ( geometry.getType () == COLLADAFW::Geometry::GEO_TYPE_MESH )
    {
        COLLADAFW::Mesh const& asMesh = static_cast< COLLADAFW::Mesh const& > ( geometry );
        
        ok = import ( importer, asMesh );
    }
    else // MCB: handle other types of geometry TODO
    {
        std::cout << "ColladaMeshObject::import(" << &geometry << ") is not a mesh, skipping..." << std::endl;
        ok = true;
    }
    return ok;
}

bool ColladaMeshObject::import ( ColladaDocumentImporter& importer, COLLADAFW::Mesh const& mesh )
{
    assert((std::cout << "MCB: ColladaMeshObject::import(COLLADAFW::Mesh) entered" << std::endl,true));
    
    bool ok = false;

    return ok;
}
    
/////////////////////////////////////////////////////////////////////
// overrides from MeshObject

void ColladaMeshObject::setMesh ( URI const& rhs )
{
    mMeshURI = rhs;
    // MCB: trigger importation of mesh content
    
    /// dbm: this is what triggers Collada download.
    /// dbm: ColladaSystem::loadDocument initiates a download using transferManager;
    /// dbm: ColladaSystem:downloadFinished is the callback
    mSystem.loadDocument ( rhs, mProxyPtr );
}
    
URI const& ColladaMeshObject::getMesh () const
{
    return mMeshURI;
}

void ColladaMeshObject::setScale ( Vector3f const& rhs )
{
    mScale = rhs;
}

Vector3f const& ColladaMeshObject::getScale () const
{
    return mScale;
}

void ColladaMeshObject::setPhysical ( PhysicalParameters const& rhs )
{
    mPhysical = rhs;
}

PhysicalParameters const& ColladaMeshObject::getPhysical () const
{
    return mPhysical;
}

} // namespace Models
} // namespace Sirikata

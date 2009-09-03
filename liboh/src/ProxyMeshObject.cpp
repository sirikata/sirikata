/*  Sirikata Object Host
 *  ProxyMeshObject.cpp
 *
 *  Copyright (c) 2009, Daniel Reiter Horn, Mark C. Barnes
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
#include <oh/ProxyMeshObject.hpp>

//#include <util/ListenerProvider.hpp>

namespace Sirikata {

ProxyMeshObject::ProxyMeshObject ( ProxyManager* man, SpaceObjectReference const& id )
    :   MeshObject (),
        MeshProvider (),
        ProxyObject ( man, id )
{

}

/////////////////////////////////////////////////////////////////////
// overrides from MeshObject

void ProxyMeshObject::setMesh ( URI const& mesh )
{
    MeshObject::setMesh ( mesh );
    MeshProvider::notify ( &MeshListener::onSetMesh, mesh );
}

URI const& ProxyMeshObject::getMesh () const
{
    return MeshObject::getMesh ();
}

void ProxyMeshObject::setScale ( Vector3f const& scale )
{
    MeshObject::setScale ( scale );
    MeshProvider::notify ( &MeshListener::onSetScale, scale );
}

Vector3f const& ProxyMeshObject::getScale () const
{
    return MeshObject::getScale ();
}

void ProxyMeshObject::setPhysical ( PhysicalParameters const& pp )
{
    MeshObject::setPhysical ( pp );
    MeshProvider::notify ( &MeshListener::onSetPhysical, pp );
}

PhysicalParameters const& ProxyMeshObject::getPhysical () const
{
    return MeshObject::getPhysical ();
}

/////////////////////////////////////////////////////////////////////
// overrides from MeshProvider


/////////////////////////////////////////////////////////////////////
// overrides from ProxyObject


}

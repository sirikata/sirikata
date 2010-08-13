/*  Sirikata Object Host
 *  ProxyMeshObject.hpp
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

#ifndef _SIRIKATA_PROXY_MESH_OBJECT_HPP_
#define _SIRIKATA_PROXY_MESH_OBJECT_HPP_

#include <sirikata/proxyobject/models/MeshObject.hpp>

#include "MeshListener.hpp"
#include "ProxyObject.hpp"
#include <sirikata/core/transfer/TransferMediator.hpp>
#include <sirikata/core/transfer/TransferPool.hpp>
#include <sirikata/proxyobject/Meshdata.hpp>

namespace Sirikata {

typedef Provider< MeshListener* > MeshProvider;

/** Represents any object with an attached mesh. */
class SIRIKATA_PROXYOBJECT_EXPORT ProxyMeshObject
:   public Models::MeshObject,
    public MeshProvider,
    public ProxyObject
{
    public:
        typedef std::tr1::shared_ptr< MeshObject > ModelObjectPtr;

        ProxyMeshObject ( ProxyManager* man, SpaceObjectReference const& id, ODP::Service* odp_service );

        void setModelObject ( ModelObjectPtr const& model );
        ModelObjectPtr const& getModelObject () const { return mModelObject; }

    protected:

    private:
        // MCB: private data for proxy (mediator) operations only
        ModelObjectPtr mModelObject;

    // interface from MeshObject
    public:
        virtual void setMesh ( URI const& rhs );

        virtual URI const& getMesh () const;

        void meshDownloaded(std::tr1::shared_ptr<Transfer::ChunkRequest>request,
            std::tr1::shared_ptr<Transfer::DenseData> response);

        virtual void setScale ( Vector3f const& rhs );
        virtual Vector3f const& getScale () const;

        virtual void setPhysical ( PhysicalParameters const& rhs );
        virtual PhysicalParameters const& getPhysical () const;

        void meshParsed(String s, Meshdata* md);

    protected:

    // interface from MeshProvider
    // MCB: Provider needs to supply a listener typedef
    public:
//        virtual void addListener ( MeshListener* p );
//        virtual void removeListener ( MeshListener* p );

    protected:
//        virtual void listenerAdded ( MeshListener* p );
//        virtual void listenerRemoved ( MeshListener* p );
//        virtual void firstListenerAdded ( MeshListener* p );
//        virtual void lastListenerRemoved ( MeshListener* p );

    // interface from ProxyObject
    public:
        virtual bool hasModelObject () const;

    protected:
//        virtual void destroyed ();

};

} // namespace Sirikata

#endif

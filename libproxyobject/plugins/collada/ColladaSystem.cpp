/*  Sirikata libproxyobject -- COLLADA Models System
 *  ColladaSystem.cpp
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

#include "ColladaSystem.hpp"

#include "ColladaErrorHandler.hpp"
#include "ColladaDocumentImporter.hpp"
#include "ColladaDocumentLoader.hpp"

#include "ColladaMeshObject.hpp"

#include <sirikata/proxyobject/ProxyMeshObject.hpp>
//#include <options/Options.hpp>

// OpenCOLLADA headers

#include "COLLADAFWRoot.h"
#include "COLLADASaxFWLLoader.h"


#include <iostream>
#include <fstream>


using namespace std;
using namespace Sirikata;
using namespace Sirikata::Transfer;

namespace Sirikata { namespace Models {

ColladaSystem::ColladaSystem ()
    :   mDocuments ()
{
    assert((std::cout << "MCB: ColladaSystem::ColladaSystem() entered" << std::endl,true));
  mTransferMediator = &(TransferMediator::getSingleton());

  mTransferPool = mTransferMediator->registerClient("ColladaGraphics");
}

ColladaSystem::~ColladaSystem ()
{
    assert((std::cout << "MCB: ColladaSystem::~ColladaSystem() entered" << std::endl,true));

}

/////////////////////////////////////////////////////////////////////

void ColladaSystem::chunkFinished(std::tr1::weak_ptr<ProxyMeshObject>(proxy), std::tr1::shared_ptr<ChunkRequest> request,
				  std::tr1::shared_ptr<const DenseData> response)
{
  if (response != NULL) {

      ColladaDocumentLoader loader(request->getMetadata().getURI(), request->getMetadata().getFingerprint(), proxy);

      SparseData data = SparseData();
      data.addValidData(response);

      Transfer::DenseDataPtr flatData = data.flatten();

      char const* buffer = reinterpret_cast<char const*>(flatData->begin());


      if (loader.load(buffer, flatData->length())) {
            // finally we can add the Product to our set of completed documents
            mDocuments.insert ( DocumentSet::value_type ( loader.getDocument () ) );
      } else {
            std::cout << "ColladaSystem::downloadFinished() loader failed!" << std::endl;
      }
  } else std::cout << "ColladaSystem::downloadFinished() failed!" << std::endl;
}

void ColladaSystem::metadataFinished(std::tr1::weak_ptr<ProxyMeshObject>(proxy), std::tr1::shared_ptr<MetadataRequest> request,
				     std::tr1::shared_ptr<RemoteFileMetadata>response)
{
  if (response != NULL) {

    const Chunk *chunk = new Chunk(response->getFingerprint(), Range(true));
    const RemoteFileMetadata metadata = *response;

    TransferRequestPtr req(new Transfer::ChunkRequest(response->getURI(), metadata, *chunk, 1.0,
						    std::tr1::bind(&ColladaSystem::chunkFinished, this, proxy, std::tr1::placeholders::_1, std::tr1::placeholders::_2)));

    mTransferPool->addRequest(req);
  }
}


void ColladaSystem::loadDocument (std::tr1::weak_ptr<ProxyMeshObject>(proxy),
    std::tr1::shared_ptr<ChunkRequest> request,
    std::tr1::shared_ptr<const DenseData> response)
{
    if (response != NULL) {

        ColladaDocumentLoader loader(request->getMetadata().getURI(), request->getMetadata().getFingerprint(), proxy);

      SparseData data = SparseData();
      data.addValidData(response);

      Transfer::DenseDataPtr flatData = data.flatten();

      char const* buffer = reinterpret_cast<char const*>(flatData->begin());


      if (loader.load(buffer, flatData->length())) {
            // finally we can add the Product to our set of completed documents
            mDocuments.insert ( DocumentSet::value_type ( loader.getDocument () ) );
      } else {
            std::cout << "ColladaSystem::downloadFinished() loader failed!" << std::endl;
      }
  } else std::cout << "ColladaSystem::downloadFinished() failed!" << std::endl;
}

/////////////////////////////////////////////////////////////////////

ColladaSystem* ColladaSystem::create ( Provider< ProxyCreationListener* >* proxyManager, String const& options )
{
    assert((std::cout << "MCB: ColladaSystem::create( " << options << ") entered" << std::endl,true));
    ColladaSystem* system ( new ColladaSystem );

    if ( system->initialize ( proxyManager, options ) )
        return system;
    delete system;
    return 0;
}

bool ColladaSystem::initialize ( Provider< ProxyCreationListener* >* proxyManager, String const& options )
{
    assert((std::cout << "MCB: ColladaSystem::initialize() entered" << std::endl,true));

    InitializeClassOptions ( "colladamodels", this, NULL );
    OptionSet::getOptions ( "colladamodels", this )->parse ( options );


    proxyManager->addListener ( this );

    return true;
}

/////////////////////////////////////////////////////////////////////
// overrides from ModelsSystem


/////////////////////////////////////////////////////////////////////
// overrides from ProxyCreationListener

void ColladaSystem::onCreateProxy ( ProxyObjectPtr proxy )
{
    assert((std::cout << "MCB: onCreateProxy (" << proxy << ") entered for ID: " << proxy->getObjectReference () << std::endl,true));

    std::tr1::shared_ptr< ProxyMeshObject > asMesh ( std::tr1::dynamic_pointer_cast< ProxyMeshObject > ( proxy ) );

    if ( asMesh )
    {
        assert((std::cout << "MCB: onCreateProxy (" << asMesh << ") entered for mesh ID: " << asMesh->getObjectReference () << std::endl,true));

        ColladaMeshObject* cmo = new ColladaMeshObject ( *this, std::tr1::shared_ptr<ProxyMeshObject>(asMesh) );
        ProxyMeshObject::ModelObjectPtr mesh ( cmo );

        // try to supply the proxy with a data model
        if ( ! proxy->hasModelObject () )
        {
            asMesh->setModelObject ( mesh );  // MCB: hoist to a common base class? with overloads??
        }
        else
        {
            // some other ModelsSystem has registered already or it's a legacy proxy
            std::cout << "MCB: onCreateProxy (" << proxy << ") claims it already has a data model?" << std::endl;
            // MCB: by listening we can peek and remap the data (usefull?)
//            asMesh->MeshProvider::addListener ( mesh );
        }
    }
    else
    {
        // MCB: check other types
    }
}

void ColladaSystem::onDestroyProxy ( ProxyObjectPtr proxy )
{
    std::tr1::shared_ptr< ProxyMeshObject > asMesh ( std::tr1::dynamic_pointer_cast< ProxyMeshObject > ( proxy ) );

    if ( asMesh )
    {
        std::cout << "MCB: onDestroyProxy (" << asMesh << ") entered for mesh URI: " << asMesh->getMesh () << std::endl;
    }
}


} // namespace Models
} // namespace Sirikata

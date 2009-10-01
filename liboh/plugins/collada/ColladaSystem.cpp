/*  Sirikata liboh -- COLLADA Models System
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

#include "ColladaMeshObject.hpp"

#include <oh/ProxyMeshObject.hpp>
//#include <oh/SimulationFactory.hpp>
//#include <options/Options.hpp>
//#include <transfer/TransferManager.hpp>

// OpenCOLLADA headers
#include "COLLADAFWRoot.h"
#include "COLLADASaxFWLLoader.h"


#include <iostream>

namespace Sirikata { namespace Models {

ColladaSystem::ColladaSystem ()
    :   mErrorHandler ( new ColladaErrorHandler ),
        mSaxLoader ( new COLLADASaxFWL::Loader ( mErrorHandler ) ),
        mDocumentImporter ( new ColladaDocumentImporter ),
        mFramework ( new COLLADAFW::Root ( mSaxLoader, mDocumentImporter ) )
{
    std::cout << "MCB: ColladaSystem::ColladaSystem() entered" << std::endl;

}
    
ColladaSystem::~ColladaSystem ()
{
    std::cout << "MCB: ColladaSystem::~ColladaSystem() entered" << std::endl;
    delete mFramework;
    delete mDocumentImporter;
    delete mSaxLoader;
    delete mErrorHandler;
}

ColladaSystem* ColladaSystem::create ( Provider< ProxyCreationListener* >* proxyManager, String const& options )
{
    std::cout << "MCB: ColladaSystem::create( " << options << ") entered" << std::endl;
    ColladaSystem* system ( new ColladaSystem );
    
    if ( system->initialize ( proxyManager, options ) )
        return system;
    delete system;
    return 0;
}

bool ColladaSystem::initialize ( Provider< ProxyCreationListener* >* proxyManager, String const& options )
{
    std::cout << "MCB: ColladaSystem::initialize() entered" << std::endl;

    proxyManager->addListener ( this );

    return true;
}

/////////////////////////////////////////////////////////////////////
// overrides from ModelsSystem


/////////////////////////////////////////////////////////////////////
// overrides from ProxyCreationListener

void ColladaSystem::onCreateProxy ( ProxyObjectPtr proxy )
{
    std::cout << "MCB: onCreateProxy (" << proxy << ") entered for ID: " << proxy->getObjectReference () << std::endl;

    std::tr1::shared_ptr< ProxyMeshObject > asMesh ( std::tr1::dynamic_pointer_cast< ProxyMeshObject > ( proxy ) );
    
    if ( asMesh )
    {
        std::cout << "MCB: onCreateProxy (" << asMesh << ") entered for mesh ID: " << asMesh->getObjectReference () << std::endl;
        
        ProxyMeshObject::ModelObjectPtr mesh ( new ColladaMeshObject ( this ) );
        
        // try to supply the proxy with a data model
        if ( ! proxy->hasModelObject () )
        {
            // MCB: trigger importation of mesh content
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

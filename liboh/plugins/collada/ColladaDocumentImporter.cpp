/*  Sirikata liboh -- COLLADA Document Importer
 *  ColladaDocumentImporter.hpp
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

#include "ColladaDocumentImporter.hpp"

#include <cassert>
#include <iostream>

namespace Sirikata { namespace Models {

ColladaDocumentImporter::ColladaDocumentImporter ( Transfer::URI const& uri )
    :   mDocument ( new ColladaDocument ( uri ) ),
        mState ( IDLE )
{
    assert((std::cout << "MCB: ColladaDocumentImporter::ColladaDocumentImporter() entered" << std::endl,true));
    
}

ColladaDocumentImporter::~ColladaDocumentImporter ()
{
    assert((std::cout << "MCB: ColladaDocumentImporter::~ColladaDocumentImporter() entered" << std::endl,true));

}

/////////////////////////////////////////////////////////////////////

ColladaDocumentPtr ColladaDocumentImporter::getDocument () const
{
    assert(mState == FINISHED);
    return mDocument;
}

void ColladaDocumentImporter::postProcess ()
{
    assert((std::cout << "MCB: ColladaDocumentImporter::postProcess() entered" << std::endl,true));

}
        
/////////////////////////////////////////////////////////////////////
// overrides from COLLADAFW::IWriter

void ColladaDocumentImporter::cancel ( COLLADAFW::String const& errorMessage )
{
    assert((std::cout << "MCB: ColladaDocumentImporter::cancel(" << errorMessage << ") entered" << std::endl,true));

    mState = CANCELLED;
}

void ColladaDocumentImporter::start ()
{
    assert((std::cout << "MCB: ColladaDocumentImporter::start() entered" << std::endl,true));

    mState = STARTED;
}

void ColladaDocumentImporter::finish ()
{
    assert((std::cout << "MCB: ColladaDocumentImporter::finish() entered" << std::endl,true));

    postProcess ();
    mState = FINISHED;    
}
    
bool ColladaDocumentImporter::writeGlobalAsset ( COLLADAFW::FileInfo const* asset )
{
    assert((std::cout << "MCB: ColladaDocumentImporter::writeGLobalAsset(" << asset << ") entered" << std::endl,true));

    bool ok = mDocument->import ( *this, *asset );

    return ok;
}

bool ColladaDocumentImporter::writeScene ( COLLADAFW::Scene const* scene )
{
    assert((std::cout << "MCB: ColladaDocumentImporter::writeScene(" << scene << ") entered" << std::endl,true));
    return true;
}


bool ColladaDocumentImporter::writeVisualScene ( COLLADAFW::VisualScene const* visualScene )
{
    assert((std::cout << "MCB: ColladaDocumentImporter::writeVisualScene(" << visualScene << ") entered" << std::endl,true));
    return true;
}


bool ColladaDocumentImporter::writeLibraryNodes ( COLLADAFW::LibraryNodes const* libraryNodes )
{
    assert((std::cout << "MCB: ColladaDocumentImporter::writeLibraryNodes(" << libraryNodes << ") entered" << std::endl,true));
    return true;
}


bool ColladaDocumentImporter::writeGeometry ( COLLADAFW::Geometry const* geometry )
{
    assert((std::cout << "MCB: ColladaDocumentImporter::writeGeometry(" << geometry << ") entered" << std::endl,true));

    bool ok = mDocument->import ( *this, *geometry );
    
    return ok;
}


bool ColladaDocumentImporter::writeMaterial ( COLLADAFW::Material const* material )
{
    assert((std::cout << "MCB: ColladaDocumentImporter::writeMaterial(" << material << ") entered" << std::endl,true));
    return true;
}


bool ColladaDocumentImporter::writeEffect ( COLLADAFW::Effect const* effect )
{
    assert((std::cout << "MCB: ColladaDocumentImporter::writeEffect(" << effect << ") entered" << std::endl,true));
    return true;
}


bool ColladaDocumentImporter::writeCamera ( COLLADAFW::Camera const* camera )
{
    assert((std::cout << "MCB: ColladaDocumentImporter::writeCamera(" << camera << ") entered" << std::endl,true));
    return true;
}


bool ColladaDocumentImporter::writeImage ( COLLADAFW::Image const* image )
{
    assert((std::cout << "MCB: ColladaDocumentImporter::writeImage(" << image << ") entered" << std::endl,true));
    return true;
}


bool ColladaDocumentImporter::writeLight ( COLLADAFW::Light const* light )
{
    assert((std::cout << "MCB: ColladaDocumentImporter::writeLight(" << light << ") entered" << std::endl,true));
    return true;
}


bool ColladaDocumentImporter::writeAnimation ( COLLADAFW::Animation const* animation )
{
    assert((std::cout << "MCB: ColladaDocumentImporter::writeAnimation(" << animation << ") entered" << std::endl,true));
    return true;
}


bool ColladaDocumentImporter::writeAnimationList ( COLLADAFW::AnimationList const* animationList )
{
    assert((std::cout << "MCB: ColladaDocumentImporter::ColladaDocumentImporter() entered" << std::endl,true));
    return true;
}


bool ColladaDocumentImporter::writeSkinControllerData ( COLLADAFW::SkinControllerData const* skinControllerData )
{
    assert((std::cout << "MCB: ColladaDocumentImporter::writeSkinControllerData(" << skinControllerData << ") entered" << std::endl,true));
    return true;
}


bool ColladaDocumentImporter::writeController ( COLLADAFW::Controller const* controller )
{
    assert((std::cout << "MCB: ColladaDocumentImporter::writeController(" << controller << ") entered" << std::endl,true));
    return true;
}

bool ColladaDocumentImporter::writeFormulas ( COLLADAFW::Formulas const* formulas )
{
    assert((std::cout << "MCB: ColladaDocumentImporter::writeFormulas(" << formulas << ") entered" << std::endl,true));
    return true;
}

bool ColladaDocumentImporter::writeKinematicsScene ( COLLADAFW::KinematicsScene const* kinematicsScene )
{
    assert((std::cout << "MCB: ColladaDocumentImporter::writeKinematicsScene(" << kinematicsScene << ") entered" << std::endl,true));
    return true;
}
    

} // namespace Models
} // namespace Sirikata

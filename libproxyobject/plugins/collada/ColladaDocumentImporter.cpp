/*  Sirikata libproxyobject -- COLLADA Document Importer
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
#include <proxyobject/Meshdata.hpp>
    
/// FIXME: need a culling strategy for this mom
std::map<std::string, Meshdata*> meshstore;
long Meshdata_counter=1000;

namespace Sirikata { namespace Models {

ColladaDocumentImporter::ColladaDocumentImporter ( Transfer::URI const& uri )
    :   mDocument ( new ColladaDocument ( uri ) ),
        mState ( IDLE )
{
    assert((std::cout << "MCB: ColladaDocumentImporter::ColladaDocumentImporter() entered, uri: " << uri << std::endl,true));
    
//    SHA256 hash = SHA256::computeDigest(uri.toString());    /// rest of system uses hash
//    lastURIString = hash.convertToHexString();
    
//    lastURIString = uri.toString();
    std::cout << "dbm debug hash uri-->" << uri.filename() << "<-- hash-->" << mDocument->getURI().toString() << "<--\n";
    meshstore[mDocument->getURI().toString()] = new Meshdata();
    meshstore[mDocument->getURI().toString()]->uri = mDocument->getURI().toString();
}

ColladaDocumentImporter::~ColladaDocumentImporter ()
{
    assert((std::cout << "MCB: ColladaDocumentImporter::~ColladaDocumentImporter() entered" << std::endl,true));

}

/////////////////////////////////////////////////////////////////////

ColladaDocumentPtr ColladaDocumentImporter::getDocument () const
{
    if(!(mState == FINISHED)) {
        SILOG(collada,fatal,"STATE != Finished reached: malformed COLLADA document");
    }
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
    if (meshstore[mDocument->getURI().toString()]->positions.size()) {
        std::cout << "dbm debug3 ::finish mProxyPtr: " << mProxyPtr << " about to call meshParsed for: " << mDocument->getURI().toString() << "\n";
        mProxyPtr->meshParsed( mDocument->getURI().toString(), meshstore[mDocument->getURI().toString()] );
    }
    mState = FINISHED;    
}
    
bool ColladaDocumentImporter::writeGlobalAsset ( COLLADAFW::FileInfo const* asset )
{
    assert((std::cout << "MCB: ColladaDocumentImporter::writeGLobalAsset(" << asset << ") entered" << std::endl,true));
    std::cout << "dbm debug writeGlobalAsset, axis: " << asset->getUpAxisType() 
            << " Yup: " << COLLADAFW::FileInfo::Y_UP 
            << " Zup: " << COLLADAFW::FileInfo::Z_UP 
            << std::endl;
    bool ok = mDocument->import ( *this, *asset );
    meshstore[mDocument->getURI().toString()]->up_axis=asset->getUpAxisType();
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
    String uri = mDocument->getURI().toString();
    assert((std::cout << "MCB: ColladaDocumentImporter::writeGeometry(" << geometry << ") entered" << std::endl,true));
    std::cout << "dbm debug Geometry original id: " << geometry->getOriginalId() << std::endl;
    std::cout << "dbm debug uri: " << uri << std::endl;
    std::cout << "dbm debug Geometry name: " << geometry->getName() << " type: " <<
    (geometry->getType()==COLLADAFW::Geometry::GEO_TYPE_MESH?"Mesh":"NotMesh") << std::endl;
    COLLADAFW::Mesh const* mesh = dynamic_cast<COLLADAFW::Mesh const*>(geometry);
    if (!mesh) {
        std::cerr << "ERROR: we only support collada Mesh\n";
        assert(false);
    }
    std::cout << "dbm debug: #faces " << mesh->getFacesCount() << std::endl;
    //COLLADAFW::MeshVertexData const v(mesh->getPositions());            // private data error!
    COLLADAFW::MeshVertexData const* verts(&(mesh->getPositions()));      // but this works?
    COLLADAFW::MeshVertexData const* norms(&(mesh->getNormals()));
    COLLADAFW::MeshVertexData const* UVs(&(mesh->getUVCoords()));
    COLLADAFW::MeshPrimitiveArray const* primitives(&(mesh->getMeshPrimitives()));
    std::cout << "dbm debug prim type: " << (*primitives)[0]->getPrimitiveType() << " #: " << primitives->getCount() << std::endl;
    if (!(*primitives)[0]->getPrimitiveType()==COLLADAFW::MeshPrimitive::TRIANGLES) {
        std::cerr << "ERROR: we only support collada MeshPrimitive::TRIANGLES\n";
        assert(false);
    }
    COLLADAFW::UIntValuesArray const* pi(&((*primitives)[0]->getPositionIndices()));
    COLLADAFW::UIntValuesArray const* ni(&((*primitives)[0]->getNormalIndices()));
    int vcnt = verts->getValuesCount();
    int ncnt = norms->getValuesCount();
    int icnt = pi->getCount();
    int uvcnt = UVs->getValuesCount();
    if (icnt != ni->getCount()) {
        std::cerr << "ERROR: position indices and normal indices differ in length!\n";
        assert(false);
    }
    std::cout << "dbm debug, length of verts: " << vcnt << " norms: " << ncnt << " indices: " << icnt << std::endl;
    COLLADAFW::FloatArray const* vdata = verts->getFloatValues();
    COLLADAFW::FloatArray const* ndata = norms->getFloatValues();
    COLLADAFW::FloatArray const* uvdata = UVs->getFloatValues();
    if (vdata && ndata) {
        float const* raw = vdata->getData();
        for (int i=0; i<vcnt; i+=3) {
            std::cout << "  dbm debug vertex: " << raw[i] <<","<< raw[i+1] <<"," << raw[i+2] << std::endl;
            meshstore[uri]->positions.push_back(Vector3f(raw[i],raw[i+1],raw[i+2]));
        }
        raw = ndata->getData();
        for (int i=0; i<ncnt; i+=3) {
            std::cout << "  dbm debug normal: " << raw[i] <<","<< raw[i+1] <<"," << raw[i+2] << std::endl;
            meshstore[uri]->normals.push_back(Vector3f(raw[i],raw[i+1],raw[i+2]));
        }
        if (uvdata) {
            raw = uvdata->getData();
            for (int i=0; i<uvcnt; i+=2) {
                std::cout << "  dbm debug uv: " << raw[i] <<","<< raw[i+1] << std::endl;
                meshstore[uri]->texUVs.push_back(Vector2f(raw[i],raw[i+1]));
            }
        }
        unsigned int const* praw = pi->getData();
        unsigned int const* nraw = ni->getData();
        for (int i=0; i<icnt; i++) {
            std::cout << "  dbm debug position index: " << praw[i] <<","<< " normal index: " << nraw[i] << std::endl;
            meshstore[uri]->position_indices.push_back(praw[i]);
            meshstore[uri]->normal_indices.push_back(nraw[i]);
        }
    }
    else {
        std::cerr << "ERROR: ColladaDocumentImporter::writeGeometry: we only support floats right now\n";
        assert(false);
    }

    bool ok = mDocument->import ( *this, *geometry );
    
    if (meshstore[uri]->texture.size()==0) {
        std::cerr << "WARNING: ColladaDocumentImporter::writeGeometry: expected texture to be initialized first\n";
    }
    return ok;
}


bool ColladaDocumentImporter::writeMaterial ( COLLADAFW::Material const* material )
{
    assert((std::cout << "MCB: ColladaDocumentImporter::writeMaterial(" << material << ") entered" << std::endl,true));
    std::cout << "dbm debug Material name: " << material->getName() << std::endl;
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
    std::cout << "dbm debug Image name: " << image->getName() << std::endl;
    std::cout << "dbm debug Image source type: " << image->getSourceType() << std::endl;
    std::cout << "dbm debug Image format: " << image->getFormat() << std::endl;
    std::cout << "dbm debug Image width, height: " << image->getWidth() <<","<< image->getHeight() << std::endl;
    std::cout << "dbm debug Image URI: " << image->getImageURI().getURIString() << std::endl;
    meshstore[mDocument->getURI().toString()]->texture = image->getImageURI().getURIString();  /// not really -- among other sins, lowercase!
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

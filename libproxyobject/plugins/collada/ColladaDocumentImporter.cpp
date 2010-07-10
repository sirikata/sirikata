/*  Sirikata libproxyobject -- COLLADA Document Importer
 *  ColladaDocumentImporter.cpp
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
#include <stack>
#include <sirikata/proxyobject/Meshdata.hpp>

#include "COLLADAFWScene.h"
#include "COLLADAFWVisualScene.h"
#include "COLLADAFWInstanceVisualScene.h"
#include "COLLADAFWLibraryNodes.h"

long Meshdata_counter=1000;

namespace Sirikata { namespace Models {

/// FIXME: need a culling strategy for this mom
std::map<std::string, Meshdata*> meshstore;

ColladaDocumentImporter::ColladaDocumentImporter ( Transfer::URI const& uri, std::tr1::weak_ptr<ProxyMeshObject>pp )
    :   mDocument ( new ColladaDocument ( uri ) ),
        mState ( IDLE ),
        mProxyPtr(pp)
{
    assert((std::cout << "MCB: ColladaDocumentImporter::ColladaDocumentImporter() entered, uri: " << uri << std::endl,true));

//    SHA256 hash = SHA256::computeDigest(uri.toString());    /// rest of system uses hash
//    lastURIString = hash.convertToHexString();

//    lastURIString = uri.toString();

    if (meshstore.find(mDocument->getURI().toString()) != meshstore.end())
        SILOG(collada,fatal,"Got duplicate request for collada document import.");

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

String ColladaDocumentImporter::documentURI() const {
    return mDocument->getURI().toString();
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

// We need this to be a non-local definition, so hide it in a namespace
namespace Collada {
struct NodeState {
    enum Modes {
        Geo = 1, // Geometry (and lights, cameras, etc)
        InstNodes = 2, // Instance nodes (requires indirection)
        Nodes = 3 // Normal child nodes
    };

    NodeState(const COLLADAFW::Node* _node, COLLADABU::Math::Matrix4 _matrix, int _child = -1, Modes _mode = Geo)
     : node(_node), matrix(_matrix), child(_child), mode(_mode)
    {}

    const COLLADAFW::Node* node;
    COLLADABU::Math::Matrix4 matrix;
    int child;
    Modes mode;
};
} // namespace Collada

void ColladaDocumentImporter::finish ()
{
    using namespace Sirikata::Models::Collada;

    assert((std::cout << "MCB: ColladaDocumentImporter::finish() entered" << std::endl,true));

    postProcess ();

    // Generate the Meshdata from all our parsed data

    // Add geometries
    // FIXME only store the geometries we need
    typedef std::map<COLLADAFW::UniqueId, int> IndicesMap;
    IndicesMap geometry_indices;
    int idx = 0;
    for(GeometryMap::const_iterator geo_it = mGeometries.begin(); geo_it != mGeometries.end(); geo_it++, idx++) {
        geometry_indices[geo_it->first] = idx;
        meshstore[documentURI()]->geometry.push_back( geo_it->second );
    }

    // Try to find the instanciated VisualScene
    VisualSceneMap::iterator vis_scene_it = mVisualScenes.find(mVisualSceneId);
    assert(vis_scene_it != mVisualScenes.end()); // FIXME
    const COLLADAFW::VisualScene* vis_scene = vis_scene_it->second;
    // Iterate through nodes. Currently we'll only output anything for nodes
    // with <instance_node> elements.
    for(int i = 0; i < vis_scene->getRootNodes().getCount(); i++) {
        const COLLADAFW::Node* rn = vis_scene->getRootNodes()[i];

        std::stack<NodeState> node_stack;
        node_stack.push( NodeState(rn, COLLADABU::Math::Matrix4::IDENTITY) );

        while(!node_stack.empty()) {
            NodeState curnode = node_stack.top();
            node_stack.pop();

            if (curnode.mode == NodeState::Geo) {
                // Transformation
                COLLADABU::Math::Matrix4 additional_xform = curnode.node->getTransformationMatrix();
                curnode.matrix = curnode.matrix * additional_xform;

                // Instance Geometries
                for(int geo_idx = 0; geo_idx < curnode.node->getInstanceGeometries().getCount(); geo_idx++) {
                    const COLLADAFW::InstanceGeometry* geo_inst = curnode.node->getInstanceGeometries()[geo_idx];
                    // FIXME handle child nodes, such as materials
                    IndicesMap::const_iterator geo_it = geometry_indices.find(geo_inst->getInstanciatedObjectId());
                    assert(geo_it != geometry_indices.end());
                    GeometryInstance new_geo_inst;
                    new_geo_inst.geometryIndex = geo_it->second;
                    new_geo_inst.transform = Matrix4x4f(curnode.matrix, Matrix4x4f::ROW_MAJOR());
                    meshstore[documentURI()]->instances.push_back(new_geo_inst);
                }

                // Instance Lights

                // Instance Cameras

                // Tell the remaining code to start processing children nodes
                curnode.child = 0;
                curnode.mode = NodeState::InstNodes;
            }
            if (curnode.mode == NodeState::InstNodes) {
                // Instance Nodes
                if (curnode.child >= curnode.node->getInstanceNodes().getCount()) {
                    curnode.child = 0;
                    curnode.mode = NodeState::Nodes;
                }
                else {
                    // Lookup the instanced node
                    COLLADAFW::UniqueId node_uniq_id = curnode.node->getInstanceNodes()[curnode.child]->getInstanciatedObjectId();
                    NodeMap::const_iterator node_it = mLibraryNodes.find(node_uniq_id);
                    assert(node_it != mLibraryNodes.end());
                    const COLLADAFW::Node* instanced_node = node_it->second;
                    // updated version of this node
                    node_stack.push( NodeState(curnode.node, curnode.matrix, curnode.child+1, curnode.mode) );
                    // And the child node
                    node_stack.push( NodeState(instanced_node, curnode.matrix) );
                }
            }
            if (curnode.mode == NodeState::Nodes) {
                // Process the next child if there are more
                if (curnode.child < curnode.node->getChildNodes().getCount()) {
                    // updated version of this node
                    node_stack.push( NodeState(curnode.node, curnode.matrix, curnode.child+1, curnode.mode) );
                    // And the child node
                    node_stack.push( NodeState(curnode.node->getChildNodes()[curnode.child], curnode.matrix) );
                }
            }
        }
    }


    // Finally, if we actually have anything for the user, ship the parsed mesh
    if (meshstore[mDocument->getURI().toString()]->instances.size() > 0) {
    //    std::tr1::shared_ptr<ProxyMeshObject>(mProxyPtr).get()->meshParsed( mDocument->getURI().toString(),
    //                                          meshstore[mDocument->getURI().toString()] );
        std::tr1::shared_ptr<ProxyMeshObject>(spp)(mProxyPtr);
        spp->meshParsed( mDocument->getURI().toString(), meshstore[mDocument->getURI().toString()] );
    }
    mState = FINISHED;
}

bool ColladaDocumentImporter::writeGlobalAsset ( COLLADAFW::FileInfo const* asset )
{
    assert((std::cout << "MCB: ColladaDocumentImporter::writeGLobalAsset(" << asset << ") entered" << std::endl,true));
    bool ok = mDocument->import ( *this, *asset );
    meshstore[mDocument->getURI().toString()]->up_axis=asset->getUpAxisType();
    return ok;
}

bool ColladaDocumentImporter::writeScene ( COLLADAFW::Scene const* scene )
{
    const COLLADAFW::InstanceVisualScene* inst_vis_scene = scene->getInstanceVisualScene();
    mVisualSceneId = inst_vis_scene->getInstanciatedObjectId();

    return true;
}


bool ColladaDocumentImporter::writeVisualScene ( COLLADAFW::VisualScene const* visualScene )
{
    mVisualScenes[visualScene->getUniqueId()] = visualScene;
    return true;
}


bool ColladaDocumentImporter::writeLibraryNodes ( COLLADAFW::LibraryNodes const* libraryNodes )
{
    for(int idx = 0; idx < libraryNodes->getNodes().getCount(); idx++) {
        const COLLADAFW::Node* node = libraryNodes->getNodes()[idx];
        mLibraryNodes[node->getUniqueId()] = node;
    }
    return true;
}


bool ColladaDocumentImporter::writeGeometry ( COLLADAFW::Geometry const* geometry )
{
    String uri = mDocument->getURI().toString();
    assert((std::cout << "MCB: ColladaDocumentImporter::writeGeometry(" << geometry << ") entered" << std::endl,true));

    COLLADAFW::Mesh const* mesh = dynamic_cast<COLLADAFW::Mesh const*>(geometry);
    if (!mesh) {
        std::cerr << "ERROR: we only support collada Mesh\n";
        assert(false);
    }

    SubMeshGeometry* submesh = new SubMeshGeometry();
    submesh->name = mesh->getName();

    //COLLADAFW::MeshVertexData const v(mesh->getPositions());            // private data error!
    COLLADAFW::MeshVertexData const* verts(&(mesh->getPositions()));      // but this works?
    COLLADAFW::MeshVertexData const* norms(&(mesh->getNormals()));
    COLLADAFW::MeshVertexData const* UVs(&(mesh->getUVCoords()));
    COLLADAFW::MeshPrimitiveArray const* primitives(&(mesh->getMeshPrimitives()));
    if (!(*primitives)[0]->getPrimitiveType()==COLLADAFW::MeshPrimitive::TRIANGLES) {
        std::cerr << "ERROR: we only support collada MeshPrimitive::TRIANGLES\n";
        assert(false);
    }
    COLLADAFW::UIntValuesArray const* pi(&((*primitives)[0]->getPositionIndices()));
    COLLADAFW::UIntValuesArray const* ni(&((*primitives)[0]->getNormalIndices()));
    COLLADAFW::UIntValuesArray const* uvi = NULL;
    if ( ((*primitives)[0]->getUVCoordIndicesArray()).getCount() > 0 ) {
        COLLADAFW::IndexList const* uv_ilist = ((*primitives)[0]->getUVCoordIndices(0)); // FIXME 0
        assert(uv_ilist->getStride() == 2);
        uvi = (&(uv_ilist->getIndices()));
    }
    int vcnt = verts->getValuesCount();
    int ncnt = norms->getValuesCount();
    unsigned int icnt = pi->getCount();
    int uvcnt = UVs->getValuesCount();
    if (icnt != ni->getCount()) {
        std::cerr << "ERROR: position indices and normal indices differ in length!\n";
        assert(false);
    }
    if (uvi != NULL && icnt != uvi->getCount()) {
        std::cerr << "ERROR: position indices and normal indices differ in length!\n";
        assert(false);
    }
    COLLADAFW::FloatArray const* vdata = verts->getFloatValues();
    COLLADAFW::FloatArray const* ndata = norms->getFloatValues();
    COLLADAFW::FloatArray const* uvdata = UVs->getFloatValues();
    if (vdata && ndata) {
        float const* raw = vdata->getData();
        for (int i=0; i<vcnt; i+=3) {
            submesh->positions.push_back(Vector3f(raw[i],raw[i+1],raw[i+2]));
        }
        raw = ndata->getData();
        for (int i=0; i<ncnt; i+=3) {
            submesh->normals.push_back(Vector3f(raw[i],raw[i+1],raw[i+2]));
        }
        if (uvdata) {
            raw = uvdata->getData();
            for (int i=0; i<uvcnt; i+=2) {
                submesh->texUVs.push_back(Vector2f(raw[i],raw[i+1]));
            }
        }
        unsigned int const* praw = pi->getData();
        unsigned int const* nraw = ni->getData();
        unsigned int const* uvraw = uvi != NULL ? uvi->getData() : NULL;
        for (unsigned int i=0; i<icnt; i++) {
            submesh->position_indices.push_back(praw[i]);
            submesh->normal_indices.push_back(nraw[i]);
            if (uvi != NULL)
                submesh->texUV_indices.push_back(uvraw[i]);
        }
    }
    else {
        std::cerr << "ERROR: ColladaDocumentImporter::writeGeometry: we only support floats right now\n";
        assert(false);
    }

    mGeometries[geometry->getUniqueId()] = submesh;

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
    meshstore[mDocument->getURI().toString()]->textures.push_back(image->getImageURI().getURIString());  /// not really -- among other sins, lowercase!
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

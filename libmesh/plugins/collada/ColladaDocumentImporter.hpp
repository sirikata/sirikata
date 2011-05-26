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

#ifndef _SIRIKATA_COLLADA_DOCUMENT_IMPORTER_
#define _SIRIKATA_COLLADA_DOCUMENT_IMPORTER_

#include "ColladaDocument.hpp"

#include "COLLADABUhash_map.h"
#include "COLLADAFWIWriter.h"
#include "COLLADAFWGeometry.h"
#include "COLLADAFWMesh.h"
#include "COLLADAFWImage.h"
#include "COLLADAFWMaterial.h"
#include "COLLADAFWMaterialBinding.h"
#include "COLLADAFWFileInfo.h"
#include "COLLADAFWNode.h"
#include "COLLADAFWColorOrTexture.h"
#include "COLLADAFWEffect.h"
#include "COLLADAFWEffectCommon.h"
#include "COLLADAFWSkinControllerData.h"
#include "COLLADAFWSkinController.h"
#include "COLLADAFWAnimationList.h"
#include <sirikata/mesh/Meshdata.hpp>

/////////////////////////////////////////////////////////////////////

namespace Sirikata {

namespace Transfer {

class URI;

}

namespace Models {

/////////////////////////////////////////////////////////////////////

/**
 *  An implementation of the OpenCOLLADA IWriter class that is responsible for importing
 *  COLLADA documents and re-Writing (i.e. converting) the data to suit the application.
 */
class SIRIKATA_PLUGIN_EXPORT ColladaDocumentImporter
    :   public COLLADAFW::IWriter
{
    public:
    explicit ColladaDocumentImporter ( Transfer::URI const& uri, const SHA256& hash );
    explicit ColladaDocumentImporter ( std::vector<Transfer::URI> uriList );

        ~ColladaDocumentImporter ();

        ColladaDocumentPtr getDocument () const;


        String documentURI() const;

        Mesh::MeshdataPtr getMeshdata() {
          return mMesh;
        }

    protected:

    private:
        ColladaDocumentImporter ( ColladaDocumentImporter const& ); // unimplemented
        ColladaDocumentImporter& operator = ( ColladaDocumentImporter const& ); // unimplemented

        void postProcess ();

        // Translates scene graph nodes into our runtime format. Part
        // of post processing step.
        void translateNodes();
        // Translates skin controllers into our runtime format and resolves
        // their various dependencies (jointes, meshes). Part of post processing
        // step.
        void translateSkinControllers();

        ColladaDocumentPtr mDocument;

        enum State { CANCELLED = -1, IDLE, STARTED, FINISHED };
        State mState;

        //returns false if everything specified was black in case all colors are black and a black rather than default material should be returned
        void makeTexture (Mesh::MaterialEffectInfo::Texture::Affecting type,
                          const COLLADAFW::MaterialBinding * binding,
                          const COLLADAFW::EffectCommon * effect,
                          const COLLADAFW::ColorOrTexture & color,
                          size_t geom_index,
                          size_t prim_index,
                          Mesh::MaterialEffectInfo::TextureList&output);
        size_t finishEffect(const COLLADAFW::MaterialBinding *binding, size_t geom_index, size_t prim_index);
    /////////////////////////////////////////////////////////////////
    // interface from COLLADAFW::IWriter
    public:
        virtual void cancel ( COLLADAFW::String const& errorMessage );
        virtual void start ();
        virtual void finish ();
        virtual bool writeGlobalAsset ( COLLADAFW::FileInfo const* asset );
        virtual bool writeScene ( COLLADAFW::Scene const* scene );
        virtual bool writeVisualScene ( COLLADAFW::VisualScene const* visualScene );
        virtual bool writeLibraryNodes ( COLLADAFW::LibraryNodes const* libraryNodes );
        virtual bool writeGeometry ( COLLADAFW::Geometry const* geometry );
        virtual bool writeMaterial ( COLLADAFW::Material const* material );
        virtual bool writeEffect ( COLLADAFW::Effect const* effect );
        virtual bool writeCamera ( COLLADAFW::Camera const* camera );
        virtual bool writeImage ( COLLADAFW::Image const* image );
        virtual bool writeLight ( COLLADAFW::Light const* light );
        virtual bool writeAnimation ( COLLADAFW::Animation const* animation );
        virtual bool writeAnimationList ( COLLADAFW::AnimationList const* animationList );
        virtual bool writeSkinControllerData ( COLLADAFW::SkinControllerData const* skinControllerData );
        virtual bool writeController ( COLLADAFW::Controller const* controller );
        virtual bool writeFormulas ( COLLADAFW::Formulas const* formulas );
        virtual bool writeKinematicsScene ( COLLADAFW::KinematicsScene const* kinematicsScene );

    protected:

        Matrix4x4f mChangeUp;
        Matrix4x4f mUnitScale; // For adjusting units to meters

        // The following keep track of the components of the scene, as
        // identified by COLLADAFW::UniqueIds.  We use these to chase indirect
        // references within the file.
        COLLADAFW::UniqueId mVisualSceneId; // Currently support loading only a
                                            // single scene

        typedef std::map<COLLADAFW::UniqueId, const COLLADAFW::VisualScene*> VisualSceneMap;
        VisualSceneMap mVisualScenes;

        typedef std::map<COLLADAFW::UniqueId, const COLLADAFW::Node*> NodeMap;
        NodeMap mLibraryNodes;
        class UniqueIdHash{public:
                size_t operator () (const COLLADAFW::UniqueId&id) const {
                    return std::tr1::hash<std::string>()(std::string(id.toAscii()));
                }
        };
        typedef std::tr1::unordered_map<COLLADAFW::UniqueId, size_t, UniqueIdHash> IndicesMap;
        typedef std::tr1::unordered_multimap<COLLADAFW::UniqueId, size_t, UniqueIdHash> IndicesMultimap;
        typedef std::tr1::unordered_map<COLLADAFW::UniqueId, COLLADAFW::UniqueId, UniqueIdHash> IdMap;

        struct OCSkinControllerData {
            Matrix4x4f bindShapeMatrix;
            ///n+1 elements where n is the number of vertices, so that we can do simple subtraction to find out how many joints influence each vertex
            std::vector<unsigned int> weightStartIndices;
            std::vector<float> weights;
            std::vector<unsigned int>jointIndices;
            std::vector<Matrix4x4f> inverseBindMatrices;
        };
        struct OCSkinController {
            COLLADAFW::UniqueId source; // The mesh this skin applies to, should
                                        // be able to get SubMeshGeometry based
                                        // on this
            COLLADAFW::UniqueId skinControllerData;
            std::vector<COLLADAFW::UniqueId> joints;
        };
        typedef std::tr1::unordered_map<COLLADAFW::UniqueId,OCSkinControllerData, UniqueIdHash> OCSkinControllerDataMap;
        typedef std::tr1::unordered_map<COLLADAFW::UniqueId,OCSkinController, UniqueIdHash> OCSkinControllerMap;
        OCSkinControllerDataMap mSkinControllerData;
        OCSkinControllerMap mSkinControllers;

        /** Generated by "sampler" tags as part of an animation. */
        struct AnimationCurve {
            std::vector<float> inputs;
            std::vector<float> outputs;
        };
        typedef std::tr1::unordered_map<COLLADAFW::UniqueId, AnimationCurve, UniqueIdHash> AnimationCurveMap;
        AnimationCurveMap mAnimationCurves;

        /** */
        typedef std::vector<COLLADAFW::AnimationList::AnimationBinding> AnimationBindings;
        typedef std::tr1::unordered_map<COLLADAFW::UniqueId, AnimationBindings, UniqueIdHash> AnimationBindingsMap;
        AnimationBindingsMap mAnimationBindings;

        Mesh::SubMeshGeometryList mGeometries;

        IndicesMultimap mGeometryMap;
        struct ExtraPrimitiveData {
            std::map<size_t, size_t> uvSetMap;
        };
        struct ExtraGeometryData {
            std::vector<ExtraPrimitiveData> primitives;
            // Reverse index from new vert index -> orig vert index, for
            // per-vertex weights since they are applied separately
            std::vector<uint32> inverseVertexIndexMap;
        };
        void setupPrim(Mesh::SubMeshGeometry::Primitive* outputPrim,
                       ExtraPrimitiveData&outputPrimExtra,
                       const COLLADAFW::MeshPrimitive*prim);
        //a list of:
        //  mappings from texture coordinate set to list indices
        //  mappings from new position indices to original (for mapping indices
        //     backward into animation weight data to make vertices that were
        //     expanded to multiple vertices get weights applied to both).
        std::vector<ExtraGeometryData> mExtraGeometryData;
        IndicesMap mLightMap;
        Mesh::LightInfoList mLights;

        IdMap mMaterialMap;
        typedef std::tr1::unordered_map<COLLADAFW::UniqueId, COLLADAFW::Effect, UniqueIdHash> ColladaEffectMap;
        ColladaEffectMap mColladaEffects;
        IndicesMap mConvertedEffects; // Index of effects already converted
                                       // which we can reuse.

        typedef std::tr1::unordered_map<COLLADAFW::UniqueId, std::string, UniqueIdHash> URIMap;
        URIMap mTextureMap;

        std::vector <COLLADAFW::EffectCommon*> mColladaClonedCommonEffects;
        //IndicesMap mEffectMap;
        Mesh::MaterialEffectInfoList mEffects;


        typedef std::tr1::unordered_map<COLLADAFW::UniqueId, size_t, UniqueIdHash> IndexMap;
        // Indices for the nodes by UniqueId. Used to find the right index after
        // the first pass has translated all nodes into the Meshdata data structure.
        IndexMap mNodeIndices;

        // Indices for joints by UniqueID. Used to find right joints
        // for node controllers. Note that these are *joint* indices,
        // not *node* indices, i.e. if we store the value x, then the
        // associate node is mesh.nodes[mesh.joints[x]].
        IndexMap mJointIndices;

        Mesh::MeshdataPtr mMesh;
};


} // namespace Models
} // namespace Sirikata

#endif // _SIRIKATA_COLLADA_DOCUMENT_IMPORTER_

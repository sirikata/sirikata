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

//#include <task/EventManager.hpp>
#include "COLLADABUhash_map.h"
#include "COLLADAFWIWriter.h"
#include "COLLADAFWGeometry.h"
#include "COLLADAFWMesh.h"
#include "COLLADAFWImage.h"
#include "COLLADAFWMaterial.h"
#include "COLLADAFWFileInfo.h"
#include "COLLADAFWNode.h"
#include <sirikata/proxyobject/ProxyMeshObject.hpp>
#include <sirikata/proxyobject/Meshdata.hpp>

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
        explicit ColladaDocumentImporter ( Transfer::URI const& uri, std::tr1::weak_ptr<ProxyMeshObject>pp );
        ~ColladaDocumentImporter ();

        ColladaDocumentPtr getDocument () const;

    protected:

    private:
        ColladaDocumentImporter ( ColladaDocumentImporter const& ); // unimplemented
        ColladaDocumentImporter& operator = ( ColladaDocumentImporter const& ); // unimplemented

        void postProcess ();

        ColladaDocumentPtr mDocument;

        enum State { CANCELLED = -1, IDLE, STARTED, FINISHED };
        State mState;

        std::tr1::weak_ptr<ProxyMeshObject>(mProxyPtr);

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
        String documentURI() const;

        // The following keep track of the components of the scene, as
        // identified by COLLADAFW::UniqueIds.  We use these to chase indirect
        // references within the file.
        COLLADAFW::UniqueId mVisualSceneId; // Currently support loading only a
                                            // single scene

        typedef std::map<COLLADAFW::UniqueId, const COLLADAFW::VisualScene*> VisualSceneMap;
        VisualSceneMap mVisualScenes;

        typedef std::map<COLLADAFW::UniqueId, const COLLADAFW::Node*> NodeMap;
        NodeMap mLibraryNodes;

        typedef std::map<COLLADAFW::UniqueId, SubMeshGeometry*> GeometryMap;
        GeometryMap mGeometries;

        typedef std::map<COLLADAFW::UniqueId, LightInfo*> LightMap;
        LightMap mLights;
};


} // namespace Models
} // namespace Sirikata

#endif // _SIRIKATA_COLLADA_DOCUMENT_IMPORTER_

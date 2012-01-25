/*  Sirikata Object Host -- Models Creation and Destruction system
 *  ModelsSystem.hpp
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

#ifndef _SIRIKATA_MODELS_SYSTEM_
#define _SIRIKATA_MODELS_SYSTEM_

#include <sirikata/core/transfer/Defs.hpp>
#include <sirikata/core/transfer/URI.hpp>
#include <sirikata/core/transfer/TransferData.hpp>
#include <sirikata/core/transfer/RemoteFileMetadata.hpp>
#include <sirikata/core/transfer/Range.hpp>

#include <sirikata/mesh/Visual.hpp>

namespace Sirikata {

/**
 * An interface for a class that is responsible for data model objects.
 */
class SIRIKATA_MESH_EXPORT ModelsSystem
{
    public:

    protected:
        ModelsSystem () {}
        ModelsSystem ( ModelsSystem const& rhs ); // not implemented
        ModelsSystem& operator = ( ModelsSystem const& rhs ); // not implemented

    public:
        virtual ~ModelsSystem () {}


        /** Check if this ModelsSystem will be able to parse the
         *  data.  This doesn't guarantee successful parsing:
         *  generally it only checks for magic numbers to see if it is
         *  likely a supported format.
         */
        virtual bool canLoad(Transfer::DenseDataPtr data) = 0;

        /** Load a mesh into a Visual object. */
        virtual Mesh::VisualPtr load(const Transfer::RemoteFileMetadata& metadata, const Transfer::Fingerprint& fp,
            Transfer::DenseDataPtr data) = 0;


        /** Convert a Visual to the format for this ModelsSystem.
         *  \param visual the Visual to save to disk
         *  \param format format hint (may or may not be used by plugin)
         *  \param vout an output stream to write the visual's data to
         *  \returns true if the conversion was successful, false otherwise
         */
        virtual bool convertVisual(const Mesh::VisualPtr& visual, const String& format, std::ostream& vout) = 0;

        /** Convert a Visual to the format for this ModelsSystem.
         *  \param visual the Visual to save to disk
         *  \param format format hint (may or may not be used by plugin)
         *  \param filename the file to save the serialized mesh to
         *  \returns true if the conversion was successful, false otherwise
         *
         *  \deprecated You should use the version of this method that takes a
         *  std::ostream instead.
         */
        virtual bool convertVisual(const Mesh::VisualPtr& visual, const String& format, const String& filename) = 0;

    protected:

};

} // namespace Sirikata

#endif // _SIRIKATA_MODELS_SYSTEM_

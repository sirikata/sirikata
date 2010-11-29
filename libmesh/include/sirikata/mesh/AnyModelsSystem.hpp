/*  Sirikata
 *  AnyModelsSystem.hpp
 *
 *  Copyright (c) 2010, Ewen Cheslack-Postava
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

#ifndef _SIRIKATA_ANY_MODELS_SYSTEM_
#define _SIRIKATA_ANY_MODELS_SYSTEM_

#include <sirikata/mesh/ModelsSystem.hpp>

namespace Sirikata {

/** AnyModelsSystem is an implementation of ModelsSystem which uses all other
 *  available implementations to handle as wide a variety of meshes as
 *  possible.  It uses all
 */
class SIRIKATA_MESH_EXPORT AnyModelsSystem : public ModelsSystem
{
  public:
    virtual ~AnyModelsSystem();

    /** Check if this ModelsSystem will be able to parse the
     *  data.  This doesn't guarantee successful parsing:
     *  generally it only checks for magic numbers to see if it is
     *  likely a supported format.
     */
    virtual bool canLoad(std::tr1::shared_ptr<const Transfer::DenseData> data);

    /** Load a mesh into a Meshdata object. */
    virtual MeshdataPtr load(const Transfer::URI& uri, const Transfer::Fingerprint& fp,
        std::tr1::shared_ptr<const Transfer::DenseData> data);

    /** Convert a Meshdata to the format for this ModelsSystem. */
    virtual bool convertMeshdata(const Meshdata& meshdata, const String& format, const String& filename);

    static ModelsSystem* create(const String& args);
  private:
    AnyModelsSystem();
    AnyModelsSystem(const AnyModelsSystem& rhs); // Not implemented
    AnyModelsSystem& operator=(const AnyModelsSystem& rhs); // Not implemented

    typedef std::map<String, ModelsSystem*> SystemsMap;
    SystemsMap mModelsSystems;
};

} // namespace Sirikata

#endif // _SIRIKATA_ANY_MODELS_SYSTEM_

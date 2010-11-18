/*  Sirikata
 *  SaveFilter.hpp
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

#include "SaveFilter.hpp"
#include <sirikata/core/options/Options.hpp>
#include <sirikata/mesh/ModelsSystemFactory.hpp>

namespace Sirikata {
namespace Mesh {

SaveFilter::SaveFilter(const String& args) {
    Sirikata::InitializeClassOptions ico("save_filter", NULL,
        new OptionValue("filename","",Sirikata::OptionValueType<String>(),"Name of file to save to."),
        new OptionValue("format","colladamodels",Sirikata::OptionValueType<String>(),"Format to save to."),
        NULL);

    OptionSet* optionSet = OptionSet::getOptions("save_filter",NULL);
    optionSet->parse(args);

    mFilename = optionSet->referenceOption("filename")->as<String>();
    mFormat = optionSet->referenceOption("format")->as<String>();
}

FilterDataPtr SaveFilter::apply(FilterDataPtr input) {
    assert(input->single());

    ModelsSystem* parser = ModelsSystemFactory::getSingleton().getConstructor("any")("");
    MeshdataPtr md = input->get();
    bool success = parser->convertMeshdata(*md.get(), mFormat, mFilename);
    if (!success) {
        std::cout << "Error saving mesh." << std::endl;
        return FilterDataPtr();
    }
    return input;
}

} // namespace Mesh
} // namespace Sirikata

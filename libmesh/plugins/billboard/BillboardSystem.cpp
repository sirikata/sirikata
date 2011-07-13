/*  Sirikata
 *  BillboardSystem.cpp
 *
 *  Copyright (c) 2011, Stanford University
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

#include "BillboardSystem.hpp"

#include <sirikata/mesh/Billboard.hpp>
// Property tree for parsing the billboard description
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

namespace Sirikata {

ModelsSystem* BillboardSystem::create(const String& args) {
    return new BillboardSystem();
}


BillboardSystem::BillboardSystem() {
}

BillboardSystem::~BillboardSystem() {
}

using namespace boost::property_tree;

bool BillboardSystem::canLoad(Transfer::DenseDataPtr data) {
    // For loading, we make sure we can parse & that there is at least a url field.
    try {
        ptree pt;
        std::stringstream data_json(data->asString());
        read_json(data_json, pt);
        return (pt.find("url") != pt.not_found());
    }
    catch(json_parser::json_parser_error exc) {
        return false;
    }

    return true;
}

Mesh::VisualPtr BillboardSystem::load(const Transfer::URI& uri, const Transfer::Fingerprint& fp,
    Transfer::DenseDataPtr data) {
    ptree pt;
    try {
        std::stringstream data_json(data->asString());
        read_json(data_json, pt);
    }
    catch(json_parser::json_parser_error exc) {
        return Mesh::VisualPtr();
    }

    String url = pt.get("url", String(""));

    Mesh::BillboardPtr result(new Mesh::Billboard());
    result->uri = uri.toString();
    result->hash = fp;
    result->image = url;
    return result;
}

bool BillboardSystem::convertVisual(const Mesh::VisualPtr& visual, const String& format, const String& filename) {
    return false;
}



} // namespace Sirikata

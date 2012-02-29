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
#include <json_spirit/json_spirit.h>

namespace Sirikata {

ModelsSystem* BillboardSystem::create(const String& args) {
    return new BillboardSystem();
}


BillboardSystem::BillboardSystem() {
}

BillboardSystem::~BillboardSystem() {
}

namespace json = json_spirit;

bool BillboardSystem::canLoad(Transfer::DenseDataPtr data) {
    // For loading, we make sure we can parse & that there is at least a url
    // field.
    json::Value parsed;
    bool success = json::read(data->asString(), parsed);
    return (
        success &&
        parsed.isObject() &&
        parsed.contains("url") &&
        parsed.get("url").type() == json::Value::STRING_TYPE);
}

Mesh::VisualPtr BillboardSystem::load(const Transfer::RemoteFileMetadata& metadata, const Transfer::Fingerprint& fp,
    Transfer::DenseDataPtr data) {

    json::Value parsed;
    bool success = json::read(data->asString(), parsed);
    if (!success) return Mesh::VisualPtr();

    String url = parsed.getString("url", "");
    float32 aspect = parsed.getReal("aspect", -1.f);
    String type = parsed.getString("facing", String("camera"));

    Mesh::BillboardPtr result(new Mesh::Billboard());
    result->uri = metadata.getURI().toString();
    result->hash = fp;
    result->image = url;
    result->aspectRatio = aspect;
    result->facing = (type == "fixed") ? Mesh::Billboard::FACING_FIXED : Mesh::Billboard::FACING_CAMERA;
    return result;
}

bool BillboardSystem::convertVisual(const Mesh::VisualPtr& meshdata, const String& format, std::ostream& vout) {
    return false;
}

bool BillboardSystem::convertVisual(const Mesh::VisualPtr& visual, const String& format, const String& filename) {
    return false;
}



} // namespace Sirikata

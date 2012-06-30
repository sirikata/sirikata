// Copyright (c) 2012 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_LIBMESH_PLY_MODEL_SYSTEM_
#define _SIRIKATA_LIBMESH_PLY_MODEL_SYSTEM_

#include <sirikata/mesh/ModelsSystem.hpp>

namespace Sirikata {

/** Implementation of ModelsSystem that loads and saves PLY files. */
class PlyModelSystem : public ModelsSystem {
public:
    PlyModelSystem();
    virtual ~PlyModelSystem ();

    virtual bool canLoad(Transfer::DenseDataPtr data);

    virtual Mesh::VisualPtr load(const Transfer::RemoteFileMetadata& metadata, const Transfer::Fingerprint& fp,
        Transfer::DenseDataPtr data);
    virtual Mesh::VisualPtr load(Transfer::DenseDataPtr data);

    virtual bool convertVisual(const Mesh::VisualPtr& visual, const String& format, std::ostream& vout);
    virtual bool convertVisual(const Mesh::VisualPtr& visual, const String& format, const String& filename);

protected:

private:

};

} // namespace Sirikata

#endif //_SIRIKATA_LIBMESH_PLY_MODEL_SYSTEM_

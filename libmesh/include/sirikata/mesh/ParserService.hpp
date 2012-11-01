// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_MESH_PARSER_SERVICE_HPP_
#define _SIRIKATA_MESH_PARSER_SERVICE_HPP_

#include <sirikata/mesh/Platform.hpp>
#include <sirikata/core/transfer/TransferMediator.hpp>
#include <sirikata/mesh/Visual.hpp>

namespace Sirikata {
namespace Mesh {

class SIRIKATA_MESH_EXPORT ParseMeshTaskInfo {
public:
    ParseMeshTaskInfo()
     : mProcess(true)
    {}

    void cancel() {
        mProcess = false;
    }

    bool process() const { return mProcess; }
private:
    bool mProcess;
};
typedef std::tr1::shared_ptr<ParseMeshTaskInfo> ParseMeshTaskHandle;

/** Simple generic interface for a service which asynchronously parses a mesh
 * and then invokes a callback with the parsed data. This can include, e.g.,
 * queueing and prioritization.
 */
class SIRIKATA_MESH_EXPORT ParserService {
public:

    typedef std::tr1::function<void(Mesh::VisualPtr)> ParseMeshCallback;

    virtual ~ParserService() {}

    /** Tries to parse a mesh. Can handle different types of meshes and tries to
     *  find the right parser using magic numbers.  If it is unable to find the
     *  right parser, returns NULL.  Otherwise, returns the parsed mesh as a
     *  Visual object.
     *  \param metadata RemoteFileMetadata describing the remote resource
     *  \param fp the fingerprint of the data, used for unique naming and passed
     *            through to the resulting mesh data
     *  \param data the contents of the
     *  \param cb callback to invoke when parsing is complete
     *
     *  \returns A handle you can use to cancel the task. You aren't required to
     *  hold onto it if you don't need to be able to cancel the request.
     */
    virtual ParseMeshTaskHandle parseMesh(const Transfer::RemoteFileMetadata& metadata, const Transfer::Fingerprint& fp, Transfer::DenseDataPtr data, bool isAggregate, ParseMeshCallback cb) = 0;
};

} // namespace Mesh
} // namespace Sirikata

#endif //_SIRIKATA_MESH_PARSER_SERVICE_HPP_

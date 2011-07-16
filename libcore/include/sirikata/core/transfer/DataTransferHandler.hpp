// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.


#ifndef _SIRIKATA_CORE_DATA_TRANSFER_HANDLER_HPP_
#define _SIRIKATA_CORE_DATA_TRANSFER_HANDLER_HPP_

#include <sirikata/core/transfer/RemoteFileMetadata.hpp>
#include <sirikata/core/transfer/TransferPool.hpp>
#include <sirikata/core/transfer/TransferHandlers.hpp>

namespace Sirikata {
namespace Transfer {

/* Implements "name lookup" for data URIs. */
class SIRIKATA_EXPORT DataNameHandler
    : public NameHandler, public AutoSingleton<DataNameHandler> {
public:
    DataNameHandler();
    ~DataNameHandler();

    /*
     * Resolves a metadata request by reading from disk
     */
    void resolve(std::tr1::shared_ptr<MetadataRequest> request, NameCallback callback);

    static DataNameHandler& getSingleton();
    static void destroy();
};

/* Implements chunk "downloading" for data URIs. */
class SIRIKATA_EXPORT DataChunkHandler
    : public ChunkHandler, public AutoSingleton<DataChunkHandler> {
public:
    DataChunkHandler();
    ~DataChunkHandler();

    /*
     * Reads the chunk referenced and calls callback when completed
     */
    void get(std::tr1::shared_ptr<RemoteFileMetadata> file,
            std::tr1::shared_ptr<Chunk> chunk, ChunkCallback callback);

    static DataChunkHandler& getSingleton();
    static void destroy();
};

} // namespace Transfer
} // namespace Sirikata

#endif //_SIRIKATA_CORE_DATA_TRANSFER_HANDLER_HPP_

/*  Sirikata Transfer -- Content Transfer management system
 *  FileTransferHandler.hpp
 *
 *  Copyright (c) 2011, Jeff Terrace
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
/*  Created on: Jun 18th, 2010 */

#ifndef SIRIKATA_FileTransferHandler_HPP__
#define SIRIKATA_FileTransferHandler_HPP__

#include <sirikata/core/transfer/RemoteFileMetadata.hpp>
#include <sirikata/core/transfer/TransferPool.hpp>
#include <sirikata/core/transfer/TransferHandlers.hpp>
#include <sirikata/core/transfer/DiskManager.hpp>

namespace Sirikata {
namespace Transfer {

/*
 * Implements name lookups via file:///
 */
class SIRIKATA_EXPORT FileNameHandler
    : public NameHandler, public AutoSingleton<FileNameHandler> {

private:
    void onReadFinished(std::tr1::shared_ptr<DenseData> fileContents,
            std::tr1::shared_ptr<MetadataRequest> request, NameCallback callback);

public:
    FileNameHandler();
    ~FileNameHandler();

    /*
     * Resolves a metadata request by reading from disk
     */
    void resolve(std::tr1::shared_ptr<MetadataRequest> request, NameCallback callback);

    static FileNameHandler& getSingleton();
    static void destroy();
};

/*
 * Implements chunk downloading via file:///
 */
class SIRIKATA_EXPORT FileChunkHandler
    : public ChunkHandler, public AutoSingleton<FileChunkHandler> {

private:
    void cache_check_callback(const SparseData* data, std::tr1::shared_ptr<RemoteFileMetadata> file,
            std::tr1::shared_ptr<Chunk> chunk, ChunkCallback callback);
    void onReadFinished(std::tr1::shared_ptr<DenseData> fileContents, std::tr1::shared_ptr<RemoteFileMetadata> file,
            std::tr1::shared_ptr<Chunk> chunk, ChunkCallback callback);

public:
    FileChunkHandler();
    ~FileChunkHandler();

    /*
     * Reads the chunk referenced and calls callback when completed
     */
    void get(std::tr1::shared_ptr<RemoteFileMetadata> file,
            std::tr1::shared_ptr<Chunk> chunk, ChunkCallback callback);

    static FileChunkHandler& getSingleton();
    static void destroy();
};

}
}

#endif

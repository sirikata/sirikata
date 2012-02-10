// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include <sirikata/core/util/Standard.hh>
#include <sirikata/core/transfer/DataTransferHandler.hpp>
#include <sirikata/core/transfer/DataURI.hpp>
// For an IOService to post to. This should be fixed to use something from
// Context...
#include <sirikata/core/transfer/HttpManager.hpp>

AUTO_SINGLETON_INSTANCE(Sirikata::Transfer::DataNameHandler);
AUTO_SINGLETON_INSTANCE(Sirikata::Transfer::DataChunkHandler);

namespace Sirikata {
namespace Transfer {

DataNameHandler& DataNameHandler::getSingleton() {
    return AutoSingleton<DataNameHandler>::getSingleton();
}
void DataNameHandler::destroy() {
    AutoSingleton<DataNameHandler>::destroy();
}


DataChunkHandler& DataChunkHandler::getSingleton() {
    return AutoSingleton<DataChunkHandler>::getSingleton();
}
void DataChunkHandler::destroy() {
    AutoSingleton<DataChunkHandler>::destroy();
}


DataNameHandler::DataNameHandler(){
}

DataNameHandler::~DataNameHandler() {
}

void DataNameHandler::resolve(std::tr1::shared_ptr<MetadataRequest> request, NameCallback callback) {
    DataURI data(request->getURI().toString());
    if (data.empty()) {
        std::tr1::shared_ptr<RemoteFileMetadata> bad;
        HttpManager::getSingleton().postCallback(std::tr1::bind(callback,bad), "DataNameHandler::resolve callback");
        return;
    }

    FileHeaders emptyHeaders;
    DenseDataPtr fileContents(new DenseData(data.data()));
    Fingerprint fp = SparseData(fileContents).computeFingerprint();

    // Only one chunk
    Range::length_type file_size = fileContents->length();
    Range whole(0, file_size, LENGTH, true);
    Chunk chunk(fp, whole);
    ChunkList chunkList;
    chunkList.push_back(chunk);

    std::tr1::shared_ptr<RemoteFileMetadata> met(
        new RemoteFileMetadata(fp, request->getURI(),
                file_size, chunkList, emptyHeaders)
    );

    HttpManager::getSingleton().postCallback(std::tr1::bind(callback, met), "DataNameHandler::resolve callback");
}



DataChunkHandler::DataChunkHandler(){
}

DataChunkHandler::~DataChunkHandler() {
}

void DataChunkHandler::get(std::tr1::shared_ptr<RemoteFileMetadata> file,
        std::tr1::shared_ptr<Chunk> chunk, ChunkCallback callback) {

    std::tr1::shared_ptr<DenseData> bad;

    //Check for null arguments
    if (!file) {
        SILOG(transfer, error, "DataChunkHandler get called with null file parameter");
        callback(bad);
        return;
    }
    if (!chunk) {
        SILOG(transfer, error, "DataChunkHandler get called with null chunk parameter");
        callback(bad);
        return;
    }

    DataURI data(file->getURI().toString());
    if (data.empty()) {
        HttpManager::getSingleton().postCallback(std::tr1::bind(callback,bad), "DataChunkHandler::resolve callback");
        return;
    }

    FileHeaders emptyHeaders;
    DenseDataPtr fileContents(new DenseData(data.data()));

    HttpManager::getSingleton().postCallback(std::tr1::bind(callback, fileContents), "DataChunkHandler::resolve callback");
}

}
}

#include <sirikata/core/transfer/FileTransferHandler.hpp>

AUTO_SINGLETON_INSTANCE(Sirikata::Transfer::FileNameHandler);
AUTO_SINGLETON_INSTANCE(Sirikata::Transfer::FileChunkHandler);

namespace Sirikata {
namespace Transfer {

FileNameHandler& FileNameHandler::getSingleton() {
    return AutoSingleton<FileNameHandler>::getSingleton();
}
void FileNameHandler::destroy() {
    AutoSingleton<FileNameHandler>::destroy();
}


FileChunkHandler& FileChunkHandler::getSingleton() {
    return AutoSingleton<FileChunkHandler>::getSingleton();
}
void FileChunkHandler::destroy() {
    AutoSingleton<FileChunkHandler>::destroy();
}


FileNameHandler::FileNameHandler(){
}

FileNameHandler::~FileNameHandler() {
}

void FileNameHandler::resolve(std::tr1::shared_ptr<MetadataRequest> request, NameCallback callback) {

    Filesystem::Path fileRequested(request->getURI().fullpath());

    std::tr1::shared_ptr<DiskManager::DiskRequest> read_req(
        new DiskManager::ReadRequest(fileRequested,
            std::tr1::bind(&FileNameHandler::onReadFinished, this, _1, request, callback)));
    DiskManager::getSingleton().addRequest(read_req);
}

void FileNameHandler::onReadFinished(std::tr1::shared_ptr<DenseData> fileContents,
        std::tr1::shared_ptr<MetadataRequest> request, NameCallback callback) {

    std::tr1::shared_ptr<RemoteFileMetadata> bad;
    if (!fileContents) {
        SILOG(transfer, error, "FileNameHandler couldn't find file '" << request->getURI().fullpath() << "'");
        callback(bad);
        return;
    }

    FileHeaders emptyHeaders;
    Fingerprint fp = SparseData(fileContents).computeFingerprint();

    //Just treat everything as a single chunk for now
    Range::length_type file_size = fileContents->length();
    Range whole(0, file_size, LENGTH, true);
    Chunk chunk(fp, whole);
    ChunkList chunkList;
    chunkList.push_back(chunk);

    SharedChunkCache::getSingleton().getCache()->addToCache(fp, fileContents);
    std::tr1::shared_ptr<RemoteFileMetadata> met(new RemoteFileMetadata(fp, request->getURI(),
                file_size, chunkList, emptyHeaders));
    callback(met);
}

FileChunkHandler::FileChunkHandler(){
}

FileChunkHandler::~FileChunkHandler() {
}

void FileChunkHandler::get(std::tr1::shared_ptr<RemoteFileMetadata> file,
        std::tr1::shared_ptr<Chunk> chunk, ChunkCallback callback) {

    //Check for null arguments
    std::tr1::shared_ptr<DenseData> bad;
    if (!file) {
        SILOG(transfer, error, "FileChunkHandler get called with null file parameter");
        callback(bad);
        return;
    }
    if (!chunk) {
        SILOG(transfer, error, "FileChunkHandler get called with null chunk parameter");
        callback(bad);
        return;
    }

    //Make sure chunk given is part of file
    bool foundIt = false;
    const ChunkList & chunks = file->getChunkList();
    for (ChunkList::const_iterator it = chunks.begin(); it != chunks.end(); it++) {
        if(*chunk == *it) {
            foundIt = true;
        }
    }
    if(!foundIt) {
        SILOG(transfer, error, "FileChunkHandler get called with chunk not present in file metadata");
        callback(bad);
        return;
    }

    //Check to see if it's in the cache first
    RemoteFileId tempId(file->getFingerprint(), URIContext());
    SharedChunkCache::getSingleton().getCache()->getData(tempId, chunk->getRange(), std::tr1::bind(
            &FileChunkHandler::cache_check_callback, this, _1, file, chunk, callback));
}

void FileChunkHandler::cache_check_callback(const SparseData* data, std::tr1::shared_ptr<RemoteFileMetadata> file,
            std::tr1::shared_ptr<Chunk> chunk, ChunkCallback callback) {
    if (data) {
        std::tr1::shared_ptr<const DenseData> flattened = data->flatten();
        callback(flattened);
    } else {
        Filesystem::Path fileRequested(file->getURI().fullpath());

        std::tr1::shared_ptr<DiskManager::DiskRequest> read_req(
            new DiskManager::ReadRequest(fileRequested,
                std::tr1::bind(&FileChunkHandler::onReadFinished, this, _1, file, chunk, callback)));
        DiskManager::getSingleton().addRequest(read_req);
    }
}

void FileChunkHandler::onReadFinished(std::tr1::shared_ptr<DenseData> fileContents, std::tr1::shared_ptr<RemoteFileMetadata> file,
        std::tr1::shared_ptr<Chunk> chunk, ChunkCallback callback) {

    std::tr1::shared_ptr<DenseData> bad;
    if (!fileContents) {
        SILOG(transfer, error, "FileChunkHandler couldn't find file '" << file->getURI().fullpath() << "'");
        callback(bad);
        return;
    }

    SharedChunkCache::getSingleton().getCache()->addToCache(file->getFingerprint(), fileContents);
    callback(fileContents);
}

}
}

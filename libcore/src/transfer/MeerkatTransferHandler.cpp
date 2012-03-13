#include <sirikata/core/util/Standard.hh>
#include <sirikata/core/options/Options.hpp>
#include <sirikata/core/transfer/TransferPool.hpp>
#include <sirikata/core/transfer/TransferHandlers.hpp>
#include <sirikata/core/transfer/RemoteFileMetadata.hpp>
#include <sirikata/core/options/Options.hpp>
#include <sirikata/core/options/CommonOptions.hpp>
#include <boost/bind.hpp>
#include <sirikata/core/transfer/MeerkatTransferHandler.hpp>
#include <sirikata/core/transfer/OAuthHttpManager.hpp>
#include <sirikata/core/transfer/URL.hpp>
#include <boost/lexical_cast.hpp>

#include <json_spirit/json_spirit.h>

AUTO_SINGLETON_INSTANCE(Sirikata::Transfer::MeerkatNameHandler);
AUTO_SINGLETON_INSTANCE(Sirikata::Transfer::MeerkatChunkHandler);
AUTO_SINGLETON_INSTANCE(Sirikata::Transfer::MeerkatUploadHandler);

namespace Sirikata {
namespace Transfer {

MeerkatNameHandler& MeerkatNameHandler::getSingleton() {
    return AutoSingleton<MeerkatNameHandler>::getSingleton();
}
void MeerkatNameHandler::destroy() {
    AutoSingleton<MeerkatNameHandler>::destroy();
}


MeerkatChunkHandler& MeerkatChunkHandler::getSingleton() {
    return AutoSingleton<MeerkatChunkHandler>::getSingleton();
}
void MeerkatChunkHandler::destroy() {
    AutoSingleton<MeerkatChunkHandler>::destroy();
}


MeerkatUploadHandler& MeerkatUploadHandler::getSingleton() {
    return AutoSingleton<MeerkatUploadHandler>::getSingleton();
}
void MeerkatUploadHandler::destroy() {
    AutoSingleton<MeerkatUploadHandler>::destroy();
}


MeerkatNameHandler::MeerkatNameHandler()
 : CDN_HOST_NAME(GetOptionValue<String>(OPT_CDN_HOST)),
   CDN_SERVICE(GetOptionValue<String>(OPT_CDN_SERVICE)),
   CDN_DNS_URI_PREFIX(GetOptionValue<String>(OPT_CDN_DNS_URI_PREFIX)),

   mCdnAddr(CDN_HOST_NAME, CDN_SERVICE)
{
}

MeerkatNameHandler::~MeerkatNameHandler() {

}

void MeerkatNameHandler::resolve(std::tr1::shared_ptr<MetadataRequest> request, NameCallback callback) {
    URL url(request->getURI());
    assert(!url.empty());

    std::string dns_uri_prefix = CDN_DNS_URI_PREFIX;
    std::string host_name = CDN_HOST_NAME;
    Network::Address cdn_addr = mCdnAddr;
    if (url.host() != "") {
        host_name = url.context().hostname();
        std::string service = url.context().service();
        if (service == "") {
            service = CDN_SERVICE;
        }
        Network::Address given_addr(host_name, service);
        cdn_addr = given_addr;
    }

    HttpManager::Headers headers;
    headers["Host"] = host_name;

    HttpManager::getSingleton().head(
        cdn_addr, dns_uri_prefix + url.fullpath(),
        std::tr1::bind(&MeerkatNameHandler::request_finished, this, _1, _2, _3, request, callback),
        headers
    );
}

void MeerkatNameHandler::request_finished(std::tr1::shared_ptr<HttpManager::HttpResponse> response,
        HttpManager::ERR_TYPE error, const boost::system::error_code& boost_error,
        std::tr1::shared_ptr<MetadataRequest> request, NameCallback callback) {

    std::tr1::shared_ptr<RemoteFileMetadata> bad;

    if (error == Transfer::HttpManager::REQUEST_PARSING_FAILED) {
        SILOG(transfer, error, "Request parsing failed during an HTTP name lookup (" << request->getURI() << ")");
        callback(bad);
        return;
    } else if (error == Transfer::HttpManager::RESPONSE_PARSING_FAILED) {
        SILOG(transfer, error, "Response parsing failed during an HTTP name lookup (" << request->getURI() << ")");
        callback(bad);
        return;
    } else if (error == Transfer::HttpManager::BOOST_ERROR) {
        SILOG(transfer, error, "A boost error happened during an HTTP name lookup (" << request->getURI() << "). Boost error = " << boost_error.message());
        callback(bad);
        return;
    } else if (error != HttpManager::SUCCESS) {
        SILOG(transfer, error, "An unknown error happened during an HTTP name lookup. (" << request->getURI() << ")");
        callback(bad);
        return;
    }

    if (response->getHeaders().size() == 0) {
        SILOG(transfer, error, "There were no headers returned during an HTTP name lookup (" << request->getURI() << ")");
        callback(bad);
        return;
    }

    HttpManager::Headers::const_iterator it;
    it = response->getHeaders().find("Content-Length");
    if (it != response->getHeaders().end()) {
        SILOG(transfer, error, "Content-Length header was present when it shouldn't be during an HTTP name lookup (" << request->getURI() << ")");
        callback(bad);
        return;
    }

    if (response->getStatusCode() != 200) {
        SILOG(transfer, error, "HTTP status code = " << response->getStatusCode() << " instead of 200 during an HTTP name lookup (" << request->getURI() << ")");
        callback(bad);
        return;
    }

    it = response->getHeaders().find("File-Size");
    if (it == response->getHeaders().end()) {
        SILOG(transfer, error, "Expected File-Size header not present during an HTTP name lookup (" << request->getURI() << ")");
        callback(bad);
        return;
    }
    std::string file_size_str = it->second;

    it = response->getHeaders().find("Hash");
    if (it == response->getHeaders().end()) {
        SILOG(transfer, error, "Expected Hash header not present during an HTTP name lookup (" << request->getURI() << ")");
        callback(bad);
        return;
    }
    std::string hash = it->second;

    if (response->getData()) {
        SILOG(transfer, error, "Body present during an HTTP name lookup (" << request->getURI() << ")");
        callback(bad);
        return;
    }

    Fingerprint fp;
    try {
        fp = Fingerprint::convertFromHex(hash);
    } catch(std::invalid_argument e) {
        SILOG(transfer, error, "Hash header didn't contain a valid Fingerprint string (" << request->getURI() << ")");
        callback(bad);
        return;
    }

    std::istringstream istream(file_size_str);
    uint64 file_size;
    istream >> file_size;
    std::ostringstream ostream;
    ostream << file_size;
    if(ostream.str() != file_size_str) {
        SILOG(transfer, error, "Error converting File-Size header string to integer (" << request->getURI() << ")");
        callback(bad);
        return;
    }

    //Just treat everything as a single chunk for now
    Range whole(0, file_size, LENGTH, true);
    Chunk chunk(fp, whole);
    ChunkList chunkList;
    chunkList.push_back(chunk);

    std::tr1::shared_ptr<RemoteFileMetadata> met(new RemoteFileMetadata(fp, request->getURI(),
            file_size, chunkList, response->getRawHeaders()));

    callback(met);
    SILOG(transfer, detailed, "done http name handler request_finished");
}

MeerkatChunkHandler::MeerkatChunkHandler()
 : CDN_HOST_NAME(GetOptionValue<String>(OPT_CDN_HOST)),
   CDN_SERVICE(GetOptionValue<String>(OPT_CDN_SERVICE)),
   CDN_DOWNLOAD_URI_PREFIX(GetOptionValue<String>(OPT_CDN_DOWNLOAD_URI_PREFIX)),
   mCdnAddr(CDN_HOST_NAME, CDN_SERVICE)
{
}

MeerkatChunkHandler::~MeerkatChunkHandler() {
}

void MeerkatChunkHandler::get(std::tr1::shared_ptr<RemoteFileMetadata> file,
        std::tr1::shared_ptr<Chunk> chunk, ChunkCallback callback) {

    //Check for null arguments
    std::tr1::shared_ptr<DenseData> bad;
    if (!file) {
        SILOG(transfer, error, "HttpChunkHandler get called with null file parameter");
        callback(bad);
        return;
    }
    if (!chunk) {
        SILOG(transfer, error, "HttpChunkHandler get called with null chunk parameter");
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
        SILOG(transfer, error, "HttpChunkHandler get called with chunk not present in file metadata");
        callback(bad);
        return;
    }

    //Check to see if it's in the cache first
    SharedChunkCache::getSingleton().getCache()->getData(file->getFingerprint(), chunk->getRange(), std::tr1::bind(
            &MeerkatChunkHandler::cache_check_callback, this, _1, file->getURI(), chunk, callback));
}

void MeerkatChunkHandler::get(std::tr1::shared_ptr<Chunk> chunk, ChunkCallback callback) {
    std::tr1::shared_ptr<DenseData> bad;
    if (!chunk) {
        SILOG(transfer, error, "HttpChunkHandler get called with null chunk parameter");
        callback(bad);
        return;
    }
    //Check to see if it's in the cache first
    SharedChunkCache::getSingleton().getCache()->getData(chunk->getHash(), chunk->getRange(), std::tr1::bind(
            &MeerkatChunkHandler::cache_check_callback, this, _1, URI("meerkat:///"), chunk, callback));
}

void MeerkatChunkHandler::cache_check_callback(const SparseData* data, const URI& uri,
            std::tr1::shared_ptr<Chunk> chunk, ChunkCallback callback) {
    if (data) {
        std::tr1::shared_ptr<const DenseData> flattened = data->flatten();
        callback(flattened);
    } else {
        URL url(uri);
        assert(!url.empty());

        std::string download_uri_prefix = CDN_DOWNLOAD_URI_PREFIX;
        std::string host_name = CDN_HOST_NAME;
        Network::Address cdn_addr = mCdnAddr;
        if (url.host() != "") {
            host_name = url.context().hostname();
            std::string service = url.context().service();
            if (service == "") {
                service = CDN_SERVICE;
            }
            Network::Address given_addr(host_name, service);
            cdn_addr = given_addr;
        }

        HttpManager::Headers headers;
        headers["Host"] = host_name;
        headers["Accept-Encoding"] = "deflate, gzip";

        bool chunkReq = false;
        if(!chunk->getRange().goesToEndOfFile() || chunk->getRange().startbyte() != 0) {
            chunkReq = true;
            headers["Range"] = "bytes=" + boost::lexical_cast<String>(chunk->getRange().startbyte()) +
                "-" + boost::lexical_cast<String>(chunk->getRange().endbyte());
        }

        HttpManager::getSingleton().get(
            cdn_addr, download_uri_prefix + "/" + chunk->getHash().convertToHexString(),
            std::tr1::bind(&MeerkatChunkHandler::request_finished, this, _1, _2, _3, uri, chunk, chunkReq, callback),
            headers
        );
    }
}

void MeerkatChunkHandler::request_finished(std::tr1::shared_ptr<HttpManager::HttpResponse> response,
        HttpManager::ERR_TYPE error, const boost::system::error_code& boost_error,
        const URI& uri, std::tr1::shared_ptr<Chunk> chunk,
        bool chunkReq, ChunkCallback callback) {

    std::tr1::shared_ptr<DenseData> bad;
    std::string reqType = "file request";
    if(chunkReq) reqType = "chunk request";

    if (error == Transfer::HttpManager::REQUEST_PARSING_FAILED) {
        SILOG(transfer, error, "Request parsing failed during an HTTP " << reqType << " (" << uri << ")");
        callback(bad);
        return;
    } else if (error == Transfer::HttpManager::RESPONSE_PARSING_FAILED) {
        SILOG(transfer, error, "Response parsing failed during an HTTP " << reqType << " (" << uri << ")");
        callback(bad);
        return;
    } else if (error == Transfer::HttpManager::BOOST_ERROR) {
        SILOG(transfer, error, "A boost error happened during an HTTP " << reqType << ". Boost error = " << boost_error.message() << " (" << uri << ")");
        callback(bad);
        return;
    } else if (error != HttpManager::SUCCESS) {
        SILOG(transfer, error, "An unknown error happened during an HTTP " << reqType << " (" << uri << ")");
        callback(bad);
        return;
    }

    if (response->getHeaders().size() == 0) {
        SILOG(transfer, error, "There were no headers returned during an HTTP " << reqType << " (" << uri << ")");
        callback(bad);
        return;
    }

    HttpManager::Headers::const_iterator it;
    it = response->getHeaders().find("Content-Length");
    if (it == response->getHeaders().end()) {
        SILOG(transfer, error, "Content-Length header was not present when it should be during an HTTP " << reqType << " (" << uri << ")");
        callback(bad);
        return;
    }

    if (response->getStatusCode() != 200) {
        SILOG(transfer, error, "HTTP status code = " << response->getStatusCode() << " instead of 200 during an HTTP " << reqType << " (" << uri << ")");
        callback(bad);
        return;
    }

    if (!response->getData()) {
        SILOG(transfer, error, "Body not present during an HTTP " << reqType << " (" << uri << ")");
        callback(bad);
        return;
    }

    it = response->getHeaders().find("Content-Range");
    if (chunkReq && it == response->getHeaders().end()) {
        SILOG(transfer, error, "Expected Content-Range header not present during an HTTP " << reqType << " (" << uri << ")");
        callback(bad);
        return;
    } else if (chunkReq) {
        std::string range_str = it->second;
        bool range_parsed = false;

        if (range_str.substr(0,6) == "bytes ") {
            range_str = range_str.substr(6);
            size_type dashPos = range_str.find('-');
            if (dashPos != std::string::npos) {
                range_str.replace(dashPos, 1, " ");

                //total file size is optional
                size_type slashPos = range_str.find('/');
                if (slashPos != std::string::npos) {
                    range_str.replace(slashPos, 1, " ");
                }

                std::istringstream instream(range_str);
                uint64 range_start;
                uint64 range_end;
                instream >> range_start;
                instream >> range_end;

                if (range_start == chunk->getRange().startbyte() &&
                        range_end == chunk->getRange().endbyte() &&
                        response->getData()->length() == chunk->getRange().length()) {
                    range_parsed = true;
                }
            }
        }

        if(!range_parsed) {
            SILOG(transfer, error, "Range header has invalid format during an HTTP " << reqType << " header='" << it->second << "'" << " (" << uri << ")");
            callback(bad);
            return;
        }
    } else if (!chunkReq && response->getData()->length() != chunk->getRange().size()) {
        SILOG(transfer, error, "Data retrieved not expected size during an HTTP "
                << reqType << " response=" << response->getData()->length() << " expected=" << chunk->getRange().size());
        callback(bad);
        return;
    }

    SILOG(transfer, detailed, "about to call addToCache with fingerprint ID = " << chunk->getHash().convertToHexString());
    SharedChunkCache::getSingleton().getCache()->addToCache(chunk->getHash(), response->getData());

    callback(response->getData());
    SILOG(transfer, detailed, "done http chunk handler request_finished");
}






MeerkatUploadHandler::MeerkatUploadHandler()
 : CDN_HOST_NAME(GetOptionValue<String>(OPT_CDN_HOST)),
   CDN_SERVICE(GetOptionValue<String>(OPT_CDN_SERVICE)),
   CDN_UPLOAD_URI_PREFIX(GetOptionValue<String>(OPT_CDN_UPLOAD_URI_PREFIX)),
   CDN_UPLOAD_STATUS_URI_PREFIX(GetOptionValue<String>(OPT_CDN_UPLOAD_STATUS_URI_PREFIX)),
   mCdnAddr(CDN_HOST_NAME, CDN_SERVICE)
{
}

MeerkatUploadHandler::~MeerkatUploadHandler() {
}

namespace {
// Return true if the service is a default value
bool ServiceIsDefault(const String& s) {
    if (!s.empty() && s != "http" && s != "80")
        return false;
    return true;
}
// Return empty if this is already empty or the default for this
// service. Otherwise, return the old value
String ServiceIfNotDefault(const String& s) {
    if (!ServiceIsDefault(s))
        return s;
    return "80";
}
}

void MeerkatUploadHandler::getServerProps(UploadRequestPtr request, Network::Address& cdn_addr, String& full_oauth_hostinfo) {
    std::string host_name = CDN_HOST_NAME;
    String host_service = ServiceIfNotDefault(CDN_SERVICE);
    cdn_addr = mCdnAddr;

    // FIXME this sort of matches name + chunk approach, but there's not upload
    // URL for a resource, so there's no way to extract all the info we would
    // need. We need some other way of specifying that through the request,
    // i.e. a more generic version of OAuthParams
    String oauth_hostname(request->oauth()->hostname);
    String oauth_service(request->oauth()->service);
    if (oauth_hostname != "") {
        host_name = oauth_hostname;
        host_service = ServiceIfNotDefault(oauth_service);
        Network::Address given_addr(host_name, host_service);
        cdn_addr = given_addr;
    }

    full_oauth_hostinfo = host_name;
    if (!ServiceIsDefault(host_service))
        full_oauth_hostinfo += ":" + host_service;
}

void MeerkatUploadHandler::upload(UploadRequestPtr request, UploadCallback callback) {
    String upload_uri_prefix = CDN_UPLOAD_URI_PREFIX;
    Network::Address cdn_addr = mCdnAddr;
    String full_oauth_hostinfo;
    getServerProps(request, cdn_addr, full_oauth_hostinfo);

    HttpManager::Headers headers;
    headers["Host"] = full_oauth_hostinfo;

    // We handle the regular request parameters in the query string. This isn't
    // ideal, but since some servers sign non-file parameters from multipart
    // form-data (despite the spec saying you only add items to the base
    // signature string if a number of specific conditions are met and they
    // aren't for those fields) we need to keep them somewhere else. This is
    // probably better anyway since it ends up resulting in signing those fields
    // as well.
    //
    // We need to include both the requested path and the user specified args in
    // the query string.
    HttpManager::QueryParameters query_params;
    // Form data consists of parameters
    for(UploadRequest::StringMap::const_iterator it = request->params().begin(); it != request->params().end(); it++) {
        query_params[it->first] = it->second;
    }
    // Requested path
    query_params["path"] = request->path();

    // The multipart data just includes files
    HttpManager::MultipartDataList multipart_post_data;
    for(UploadRequest::StringMap::const_iterator it = request->files().begin(); it != request->files().end(); it++) {
        // Field, value, filename. We assume the field and filename are the same
        multipart_post_data.push_back( HttpManager::MultipartData(it->first, it->second, it->first) );
    }

    OAuthHttpManager oauth_http(request->oauth());
    oauth_http.postMultipartForm(
        cdn_addr, upload_uri_prefix,
        multipart_post_data,
        std::tr1::bind(&MeerkatUploadHandler::request_finished, this, _1, _2, _3, request, callback),
        headers, query_params
    );
}

void MeerkatUploadHandler::request_finished(std::tr1::shared_ptr<HttpManager::HttpResponse> response,
    HttpManager::ERR_TYPE error, const boost::system::error_code& boost_error,
    UploadRequestPtr request, UploadCallback callback) {

    Transfer::URI bad;

    if (error == Transfer::HttpManager::REQUEST_PARSING_FAILED) {
        SILOG(transfer, error, "Request parsing failed during an HTTP upload (" << request->getIdentifier() << ")");
        callback(bad);
        return;
    } else if (error == Transfer::HttpManager::RESPONSE_PARSING_FAILED) {
        SILOG(transfer, error, "Response parsing failed during an HTTP upload (" << request->getIdentifier() << ")");
        callback(bad);
        return;
    } else if (error == Transfer::HttpManager::BOOST_ERROR) {
        SILOG(transfer, error, "A boost error happened during an HTTP upload (" << request->getIdentifier() << "). Boost error = " << boost_error.message());
        callback(bad);
        return;
    } else if (error != HttpManager::SUCCESS) {
        SILOG(transfer, error, "An unknown error happened during an HTTP upload. (" << request->getIdentifier() << ")");
        callback(bad);
        return;
    }

    if (response->getHeaders().size() == 0) {
        SILOG(transfer, error, "There were no headers returned during an HTTP upload (" << request->getIdentifier() << ")");
        callback(bad);
        return;
    }

    if (response->getStatusCode() != 200) {
        SILOG(transfer, error, "HTTP status code = " << response->getStatusCode() << " instead of 200 during an HTTP upload (" << request->getIdentifier() << ")");
        callback(bad);
        return;
    }    

    DenseDataPtr response_data = response->getData();

    // Parse the JSON response
    namespace json = json_spirit;
    json::Value parsed;
    if (!json::read(response_data->asString(), parsed)) {
        SILOG(transfer, error, "Failed to parse upload response as JSON: " << response_data->asString() << " (" << request->getIdentifier() << ")");
        return;
    }

    // Basic success check
    if (!parsed.contains("success") || !parsed.get("success").isBool()) {
        SILOG(transfer, error, "Upload response didn't contain boolean success flag (" << request->getIdentifier() << ")");
        callback(bad);
        return;
    }
    if (!parsed.getBool("success")) {
        String reported_error = parsed.getString("error", "(No error message reported)");
        SILOG(transfer, error, "Upload failed: " << reported_error << " (" << request->getIdentifier() << ")");
        callback(bad);
        return;
    }

    // Task ID
    if (!parsed.contains("task_id") || !parsed.get("task_id").isString()) {
        SILOG(transfer, error, "Upload indicated initial success, but couldn't find task (" << request->getIdentifier() << ")");
        callback(bad);
        return;
    }

    String reported_task_id = parsed.getString("task_id");
    SILOG(transfer, detailed, "Upload succeeded, starting to track task id = " << reported_task_id << " (" << request->getIdentifier() << ")");
    // 30 retries * .5s = 15s
    requestStatus(request, reported_task_id, callback, 30);
}

void MeerkatUploadHandler::requestStatus(UploadRequestPtr request, const String& task_id, UploadCallback callback, int32 retries) {
    String upload_status_uri_prefix = CDN_UPLOAD_STATUS_URI_PREFIX;
    Network::Address cdn_addr = mCdnAddr;
    String full_oauth_hostinfo;
    getServerProps(request, cdn_addr, full_oauth_hostinfo);

    HttpManager::Headers headers;
    headers["Host"] = full_oauth_hostinfo;

    HttpManager::QueryParameters query_params;
    query_params["api"] = "true";
    //TODO(ewencp) This is kind of gross -- we need to extract the username
    //parameter because this is currently required by the CDN
    if (request->params().find("username") != request->params().end())
        query_params["username"] = request->params().find("username")->second;

    HttpManager::getSingleton().get(
        cdn_addr, upload_status_uri_prefix + "/" + task_id,
        std::tr1::bind(&MeerkatUploadHandler::handleRequestStatusResult, this, _1, _2, _3, request, task_id, callback, retries),
        headers, query_params
    );
}

void MeerkatUploadHandler::handleRequestStatusResult(
    std::tr1::shared_ptr<HttpManager::HttpResponse> response,
    HttpManager::ERR_TYPE error, const boost::system::error_code& boost_error,
    UploadRequestPtr request, const String& task_id, UploadCallback callback,
    int32 retries
)
{
    Transfer::URI bad;

    if (error == Transfer::HttpManager::REQUEST_PARSING_FAILED) {
        SILOG(transfer, error, "Request parsing failed during an HTTP upload status check (" << request->getIdentifier() << ")");
        callback(bad);
        return;
    } else if (error == Transfer::HttpManager::RESPONSE_PARSING_FAILED) {
        SILOG(transfer, error, "Response parsing failed during an HTTP upload status check (" << request->getIdentifier() << ")");
        callback(bad);
        return;
    } else if (error == Transfer::HttpManager::BOOST_ERROR) {
        SILOG(transfer, error, "A boost error happened during an HTTP upload status check (" << request->getIdentifier() << "). Boost error = " << boost_error.message());
        callback(bad);
        return;
    } else if (error != HttpManager::SUCCESS) {
        SILOG(transfer, error, "An unknown error happened during an HTTP upload status check. (" << request->getIdentifier() << ")");
        callback(bad);
        return;
    }

    if (response->getStatusCode() != 200) {
        SILOG(transfer, error, "HTTP status code = " << response->getStatusCode() << " instead of 200 during an HTTP upload status check (" << request->getIdentifier() << ")");
        callback(bad);
        return;
    }

    DenseDataPtr response_data = response->getData();
    // Parse the JSON response
    namespace json = json_spirit;
    json::Value parsed;
    if (!json::read(response_data->asString(), parsed)) {
        SILOG(transfer, error, "Failed to parse upload status check response as JSON: " << response_data->asString() << " (" << request->getIdentifier() << ")");
        return;
    }

    if (!parsed.contains("state") || !parsed.get("state").isString()) {
        SILOG(transfer, error, "No state reported in status check (" << request->getIdentifier() << ")");
        callback(bad);
        return;
    }

    String reported_state = parsed.getString("state");
    if (reported_state != "SUCCESS" && reported_state != "FAILURE") {
        // Some other notice, like PENDING, keep waiting until we get something
        // different
        SILOG(transfer, detailed, "Upload still processing: " << reported_state << " (" << request->getIdentifier() << ")");
        HttpManager::getSingleton().postCallback(
            Duration::seconds(0.5),
            std::tr1::bind(&MeerkatUploadHandler::requestStatus, this, request, task_id, callback, --retries),
            "MeerkatUploadHandler::requestStatus"
        );
        return;
    }

    if (reported_state == "FAILURE") {
        SILOG(transfer, error, "Upload failed during processing (" << request->getIdentifier() << ")");
        callback(bad);
        return;
    }

    String asset_path = parsed.getString("path");
    SILOG(transfer, detailed, "Upload succeeded and got path " << asset_path << " (" << request->getIdentifier() << ")");

    // Construct the meerkat:// URL for this asset. We need server
    // info to ensure we specify the right server
    Network::Address cdn_addr = mCdnAddr;
    String full_oauth_hostinfo;
    getServerProps(request, cdn_addr, full_oauth_hostinfo);
    // TODO(ewencp) we need the full URI to the model here, maybe
    // automatically selecting optimized?
    String generated_uri_str = "meerkat://" + full_oauth_hostinfo + asset_path;

    Transfer::URI generated_uri(generated_uri_str);
    callback(generated_uri);
    SILOG(transfer, detailed, "done http upload handler request_finished");
}

}
}

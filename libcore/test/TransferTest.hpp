/*  Sirikata Transfer -- Content Distribution Network
 *  TransferTest.hpp
 *
 *  Copyright (c) 2010, Jeff Terrace
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
/*  Created on: Jan 18, 2010 */

#include <cxxtest/TestSuite.h>

#include <sirikata/core/util/Thread.hpp>

#include <sirikata/core/transfer/URI.hpp>

#include <sirikata/core/transfer/TransferPool.hpp>
#include <sirikata/core/transfer/RemoteFileMetadata.hpp>
#include <sirikata/core/transfer/TransferMediator.hpp>

#include <sirikata/core/transfer/DiskManager.hpp>
#include <sirikata/core/transfer/LRUPolicy.hpp>

#include <sirikata/core/network/Address.hpp>
#include <sirikata/core/transfer/HttpManager.hpp>

#include <sirikata/core/options/CommonOptions.hpp>

#include <string>

using namespace Sirikata;
using boost::asio::ip::tcp;

class HttpTransferTest : public CxxTest::TestSuite {

public:

    boost::condition_variable mDone;
    boost::mutex mMutex;
    std::tr1::shared_ptr<Transfer::HttpManager::HttpResponse> mHttpResponse;

    boost::mutex mNumCbsMutex;
    int mNumCbs;

    void setUp() {
        InitOptions();
        FakeParseOptions();
    }

    void tearDown() {

    }

    void testRawHttp() {

        using std::tr1::placeholders::_1;
        using std::tr1::placeholders::_2;
        using std::tr1::placeholders::_3;

        Network::Address addr("cdn.sirikata.com", "http");
        std::map<std::string, std::string>::const_iterator it;
        std::ostringstream request_stream;
        boost::unique_lock<boost::mutex> lock(mMutex);


        /*
         * For HEAD name request, check for File-Size and Hash headers
         * and make sure body is null, content length is not present,
         * http status is 200
         */
        request_stream.str("");
        request_stream << "HEAD /dns/global/jkusnerz/Boulder_Spire_01.mesh HTTP/1.1\r\n";
        request_stream << "Host: cdn.sirikata.com\r\n";
        request_stream << "Accept: */*\r\n";
        request_stream << "Connection: close\r\n\r\n";

        SILOG(transfer, debug, "Issuing head metadata request");
        Transfer::HttpManager::getSingleton().makeRequest(addr, Transfer::HttpManager::HEAD, request_stream.str(),
                std::tr1::bind(&HttpTransferTest::request_finished, this, _1, _2, _3));
        mDone.wait(lock);

        TS_ASSERT(mHttpResponse);
        if(mHttpResponse) {
            it = mHttpResponse->getHeaders().find("Content-Length");
            Transfer::TransferMediator * mTransferMediator;
            std::tr1::shared_ptr<Transfer::TransferPool> mTransferPool;
            TS_ASSERT(it == mHttpResponse->getHeaders().end());
            TS_ASSERT(mHttpResponse->getStatusCode() == 200);
            TS_ASSERT(mHttpResponse->getHeaders().size() != 0);
            it = mHttpResponse->getHeaders().find("File-Size");
            TS_ASSERT(it != mHttpResponse->getHeaders().end());
            it = mHttpResponse->getHeaders().find("Hash");
            TS_ASSERT(it != mHttpResponse->getHeaders().end());
            TS_ASSERT( !(mHttpResponse->getData()) );
        }



        /*
         * For HEAD file request, make sure body is null,
         * content length is present and correct, status code is 200
         */
        request_stream.str("");
        request_stream << "HEAD /files/global/ddde4f8bed9a8bc97d8cbd4137c63efd5e625fabbbe695bc26756a3f5f430aa4 HTTP/1.1\r\n";
        request_stream << "Host: cdn.sirikata.com\r\n";
        request_stream << "Accept: */*\r\n";
        request_stream << "Connection: close\r\n\r\n";

        SILOG(transfer, debug, "Issuing head file request");
        Transfer::HttpManager::getSingleton().makeRequest(addr, Transfer::HttpManager::HEAD, request_stream.str(),
                std::tr1::bind(&HttpTransferTest::request_finished, this, _1, _2, _3));
        mDone.wait(lock);

        TS_ASSERT(mHttpResponse);
        if(mHttpResponse) {
            it = mHttpResponse->getHeaders().find("Content-Length");
            TS_ASSERT(it != mHttpResponse->getHeaders().end());
            TS_ASSERT(mHttpResponse->getContentLength() == 11650);
            TS_ASSERT(mHttpResponse->getStatusCode() == 200);
            TS_ASSERT(mHttpResponse->getHeaders().size() != 0);
            TS_ASSERT( !(mHttpResponse->getData()) );
        }



        /*
         * For GET file request, check content length is present,
         * content length = data size, http status code 200,
         * check content length = correct size of file
         */
        request_stream.str("");
        request_stream << "GET /files/global/ddde4f8bed9a8bc97d8cbd4137c63efd5e625fabbbe695bc26756a3f5f430aa4 HTTP/1.1\r\n";
        request_stream << "Host: cdn.sirikata.com\r\n";
        request_stream << "Accept: */*\r\n";
        request_stream << "Connection: close\r\n\r\n";

        SILOG(transfer, debug, "Issuing get file request");
        Transfer::HttpManager::getSingleton().makeRequest(addr, Transfer::HttpManager::GET, request_stream.str(),
                std::tr1::bind(&HttpTransferTest::request_finished, this, _1, _2, _3));
        mDone.wait(lock);

        TS_ASSERT(mHttpResponse);
        if(mHttpResponse) {
            TS_ASSERT(mHttpResponse->getHeaders().size() != 0);
            it = mHttpResponse->getHeaders().find("Content-Length");
            TS_ASSERT(it != mHttpResponse->getHeaders().end());
            TS_ASSERT(mHttpResponse->getStatusCode() == 200);
            TS_ASSERT(mHttpResponse->getData());
            TS_ASSERT(mHttpResponse->getData()->length() == (uint64)mHttpResponse->getContentLength());
            TS_ASSERT(mHttpResponse->getContentLength() == 11650);
        }




        /*
         * For GET file range request, check content length is present,
         * content length = range size, http status code 200
         */
        request_stream.str("");
        request_stream << "GET /files/global/ddde4f8bed9a8bc97d8cbd4137c63efd5e625fabbbe695bc26756a3f5f430aa4 HTTP/1.1\r\n";
        request_stream << "Range: bytes=10-20\r\n";
        request_stream << "Host: cdn.sirikata.com\r\n";
        request_stream << "Accept: */*\r\n";
        request_stream << "Connection: close\r\n\r\n";

        SILOG(transfer, debug, "Issuing get file range request");
        Transfer::HttpManager::getSingleton().makeRequest(addr, Transfer::HttpManager::GET, request_stream.str(),
                std::tr1::bind(&HttpTransferTest::request_finished, this, _1, _2, _3));
        mDone.wait(lock);

        TS_ASSERT(mHttpResponse);
        if(mHttpResponse) {
            TS_ASSERT(mHttpResponse->getHeaders().size() != 0);
            it = mHttpResponse->getHeaders().find("Content-Length");
            TS_ASSERT(it != mHttpResponse->getHeaders().end());
            TS_ASSERT(mHttpResponse->getStatusCode() == 200);
            TS_ASSERT(mHttpResponse->getData());
            TS_ASSERT(mHttpResponse->getData()->length() == (uint64)mHttpResponse->getContentLength());
            TS_ASSERT(mHttpResponse->getContentLength() == 11);
            SILOG(transfer, debug, "content length is " << mHttpResponse->getContentLength());
            SILOG(transfer, debug, "data length is " << mHttpResponse->getData()->length());
        }





        /*
         * Do a GET request with no Connection: close to test persistent
         * connections. check content length is present,
         * content length = data size, http status code 200,
         * check content length = correct size of file
         */
        request_stream.str("");
        request_stream << "GET /files/global/ddde4f8bed9a8bc97d8cbd4137c63efd5e625fabbbe695bc26756a3f5f430aa4 HTTP/1.1\r\n";
        request_stream << "Host: cdn.sirikata.com\r\n";
        request_stream << "Accept: */*\r\n\r\n";

        SILOG(transfer, debug, "Issuing persistent get file request");
        Transfer::HttpManager::getSingleton().makeRequest(addr, Transfer::HttpManager::GET, request_stream.str(),
                std::tr1::bind(&HttpTransferTest::request_finished, this, _1, _2, _3));
        mDone.wait(lock);

        TS_ASSERT(mHttpResponse);
        if(mHttpResponse) {
            TS_ASSERT(mHttpResponse->getHeaders().size() != 0);
            it = mHttpResponse->getHeaders().find("Content-Length");
            TS_ASSERT(it != mHttpResponse->getHeaders().end());
            TS_ASSERT(mHttpResponse->getStatusCode() == 200);
            TS_ASSERT(mHttpResponse->getData());
            TS_ASSERT(mHttpResponse->getData()->length() == (uint64)mHttpResponse->getContentLength());
            TS_ASSERT(mHttpResponse->getContentLength() == 11650);
        }

        /*
         * Do a GET request with accept-encoding set to test compression
         * check content length is present, content length = data size,
         * http status code 200, Content-Encoding = gzip
         */
        request_stream.str("");
        request_stream << "GET /files/global/ddde4f8bed9a8bc97d8cbd4137c63efd5e625fabbbe695bc26756a3f5f430aa4 HTTP/1.1\r\n";
        request_stream << "Host: cdn.sirikata.com\r\n";
        request_stream << "Accept: */*\r\n";
        request_stream << "Accept-Encoding: deflate, gzip\r\n\r\n";

        SILOG(transfer, debug, "Issuing compressed get file request");
        Transfer::HttpManager::getSingleton().makeRequest(addr, Transfer::HttpManager::GET, request_stream.str(),
                std::tr1::bind(&HttpTransferTest::request_finished, this, _1, _2, _3));
        mDone.wait(lock);

        TS_ASSERT(mHttpResponse);
        if(mHttpResponse) {
            TS_ASSERT(mHttpResponse->getHeaders().size() != 0);
            it = mHttpResponse->getHeaders().find("Content-Length");
            TS_ASSERT(it != mHttpResponse->getHeaders().end());
            it = mHttpResponse->getHeaders().find("Content-Encoding");
            TS_ASSERT(it != mHttpResponse->getHeaders().end());
            TS_ASSERT(it->second == "gzip");
            TS_ASSERT(mHttpResponse->getStatusCode() == 200);
            TS_ASSERT(mHttpResponse->getData());
            TS_ASSERT(mHttpResponse->getData()->length() == (uint64)mHttpResponse->getContentLength());
            TS_ASSERT(mHttpResponse->getContentLength() == 11650);
        }


        /*
         * Do a GET request with accept-encoding set AND make it a range request
         * check content length present, content length = range size,
         * http status code 200, Content-Encoding = gzip, check actual bytes
         */
        request_stream.str("");
        request_stream << "GET /files/global/ddde4f8bed9a8bc97d8cbd4137c63efd5e625fabbbe695bc26756a3f5f430aa4 HTTP/1.1\r\n";
        request_stream << "Host: cdn.sirikata.com\r\n";
        request_stream << "Range: bytes=10-20\r\n";
        request_stream << "Accept: */*\r\n";
        request_stream << "Accept-Encoding: deflate, gzip\r\n\r\n";

        SILOG(transfer, debug, "Issuing compressed range get file request");
        Transfer::HttpManager::getSingleton().makeRequest(addr, Transfer::HttpManager::GET, request_stream.str(),
                std::tr1::bind(&HttpTransferTest::request_finished, this, _1, _2, _3));
        mDone.wait(lock);

        TS_ASSERT(mHttpResponse);
        if(mHttpResponse) {
            TS_ASSERT(mHttpResponse->getHeaders().size() != 0);
            it = mHttpResponse->getHeaders().find("Content-Length");
            TS_ASSERT(it != mHttpResponse->getHeaders().end());
            it = mHttpResponse->getHeaders().find("Content-Encoding");
            TS_ASSERT(it != mHttpResponse->getHeaders().end());
            TS_ASSERT(it->second == "gzip");
            TS_ASSERT(mHttpResponse->getStatusCode() == 200);
            TS_ASSERT(mHttpResponse->getData());
            TS_ASSERT(mHttpResponse->getData()->length() == (uint64)mHttpResponse->getContentLength());
            TS_ASSERT(mHttpResponse->getContentLength() == 11);
            TS_ASSERT(*mHttpResponse->getData()->dataAt(0) == 'i');
            TS_ASSERT(*mHttpResponse->getData()->dataAt(1) == 'a');
            TS_ASSERT(*mHttpResponse->getData()->dataAt(2) == 'l');
            TS_ASSERT(*mHttpResponse->getData()->dataAt(3) == 'i');
            TS_ASSERT(*mHttpResponse->getData()->dataAt(4) == 'z');
            TS_ASSERT(*mHttpResponse->getData()->dataAt(5) == 'e');
            TS_ASSERT(*mHttpResponse->getData()->dataAt(6) == 'r');
            TS_ASSERT(*mHttpResponse->getData()->dataAt(7) == '_');
            TS_ASSERT(*mHttpResponse->getData()->dataAt(8) == 'v');
            TS_ASSERT(*mHttpResponse->getData()->dataAt(9) == '1');
            TS_ASSERT(*mHttpResponse->getData()->dataAt(10) == '.');
        }




        /*
         * Do a GET request with accept-encoding set to test compression
         * and then do the same request with compression off to compare
         * and make sur ethey are equal
         */
        request_stream.str("");
        request_stream << "GET /files/global/af8e8a2c90802c7639e2026ce5f03f253130a7fe5ee79da0397416d4fc393c47 HTTP/1.1\r\n";
        request_stream << "Host: cdn.sirikata.com\r\n";
        request_stream << "Accept: */*\r\n";
        request_stream << "Accept-Encoding: deflate, gzip\r\n\r\n";

        SILOG(transfer, debug, "Issuing compressed get file request");
        Transfer::HttpManager::getSingleton().makeRequest(addr, Transfer::HttpManager::GET, request_stream.str(),
                std::tr1::bind(&HttpTransferTest::request_finished, this, _1, _2, _3));
        mDone.wait(lock);

        std::tr1::shared_ptr<Transfer::HttpManager::HttpResponse> compressed = mHttpResponse;

        request_stream.str("");
        request_stream << "GET /files/global/af8e8a2c90802c7639e2026ce5f03f253130a7fe5ee79da0397416d4fc393c47 HTTP/1.1\r\n";
        request_stream << "Host: cdn.sirikata.com\r\n";
        request_stream << "Accept: */*\r\n\r\n";

        SILOG(transfer, debug, "Issuing uncompresed get file request");
        Transfer::HttpManager::getSingleton().makeRequest(addr, Transfer::HttpManager::GET, request_stream.str(),
                std::tr1::bind(&HttpTransferTest::request_finished, this, _1, _2, _3));
        mDone.wait(lock);

        std::tr1::shared_ptr<Transfer::HttpManager::HttpResponse> uncompressed = mHttpResponse;

        TS_ASSERT(compressed);
        TS_ASSERT(uncompressed);
        if(compressed && uncompressed) {
            //make sure content length and status code are the same
            TS_ASSERT(compressed->getContentLength() == uncompressed->getContentLength());
            TS_ASSERT(compressed->getStatusCode() == uncompressed->getStatusCode());

            //get headers
            typedef std::map<std::string, std::string> HeaderMapType;
            const HeaderMapType& compressedHeaders = compressed->getHeaders();
            const HeaderMapType& uncompressedHeaders = uncompressed->getHeaders();

            //loop through compressed headers
            for(HeaderMapType::const_iterator it = compressedHeaders.begin(); it != compressedHeaders.end(); it++) {
                if(it->first != "Content-Encoding" && it->first != "Vary" && it->first != "Content-Length" && it->first != "Date") {
                    HeaderMapType::const_iterator findOther = uncompressedHeaders.find(it->first);
                    TS_ASSERT(findOther != uncompressedHeaders.end());
                    if(findOther == uncompressedHeaders.end()) {
                        SILOG(transfer, error, "Different header = " << it->first << ": " << it->second);
                    } else {
                        TS_ASSERT(findOther->second == it->second);
                        if(findOther->second != it->second) {
                            SILOG(transfer, error, "Header " << it->first <<
                                    " differs. compressed = " << it->second <<
                                    " while uncompressed = " << findOther->second);
                        }
                    }
                }
            }

            //now loop through uncompressed headers
            for(HeaderMapType::const_iterator it = uncompressedHeaders.begin(); it != uncompressedHeaders.end(); it++) {
                if(it->first != "Content-Encoding" && it->first != "Vary" && it->first != "Content-Length" && it->first != "Date") {
                    HeaderMapType::const_iterator findOther = compressedHeaders.find(it->first);
                    TS_ASSERT(findOther != compressedHeaders.end());
                    if(findOther == compressedHeaders.end()) {
                        SILOG(transfer, error, "Different header = " << it->first << ": " << it->second);
                    } else {
                        TS_ASSERT(findOther->second == it->second);
                        if(findOther->second != it->second) {
                            SILOG(transfer, error, "Header " << it->first <<
                                    " differs. uncompressed = " << it->second <<
                                    " while compressed = " << findOther->second);
                        }
                    }
                }
            }

            //now compare data byte by byte
            std::tr1::shared_ptr<Sirikata::Transfer::DenseData> compressedData = compressed->getData();
            std::tr1::shared_ptr<Sirikata::Transfer::DenseData> uncompressedData = uncompressed->getData();
            TS_ASSERT(*compressedData == *uncompressedData);
            TS_ASSERT(compressedData->length() == uncompressedData->length());
            for(int i=0; i<compressed->getContentLength(); i++) {
                TS_ASSERT(*(compressedData->dataAt(i)) == *(uncompressedData->dataAt(i)));
            }

            //now turn into sparse, then flatten and compare again
            Transfer::SparseData compressedSparse = Transfer::SparseData();
            Transfer::SparseData uncompressedSparse = Transfer::SparseData();
            compressedSparse.addValidData(compressedData);
            uncompressedSparse.addValidData(uncompressedData);

            Transfer::DenseDataPtr compressedFlattened = compressedSparse.flatten();
            Transfer::DenseDataPtr uncompressedFlattened = uncompressedSparse.flatten();
            TS_ASSERT(*compressedFlattened == *uncompressedFlattened);
            TS_ASSERT(compressedFlattened->length() == uncompressedFlattened->length());
            for(uint32 i=0; i<compressedFlattened->length(); i++) {
                TS_ASSERT(*(compressedFlattened->dataAt(i)) == *(uncompressedFlattened->dataAt(i)));
            }
        }



        /*
         * Now, let's plug in a bunch of persistent connections (no connection:close)
         * all at once to stress test
         */
        mNumCbs = 20;
        for(int i=0; i<20; i++) {
            request_stream.str("");
            request_stream << "GET /files/global/ddde4f8bed9a8bc97d8cbd4137c63efd5e625fabbbe695bc26756a3f5f430aa4 HTTP/1.1\r\n";
            request_stream << "Host: cdn.sirikata.com\r\n";
            request_stream << "Accept: */*\r\n";
            request_stream << "Accept-Encoding: deflate, gzip\r\n";
            request_stream << "\r\n";

            SILOG(transfer, debug, "Issuing persistent get file request #" << i+1);
            Transfer::HttpManager::getSingleton().makeRequest(addr, Transfer::HttpManager::GET, request_stream.str(),
                    std::tr1::bind(&HttpTransferTest::multi_request_finished, this, _1, _2, _3));
        }

        mDone.wait(lock);


    }

    void multi_request_finished(std::tr1::shared_ptr<Transfer::HttpManager::HttpResponse> response,
            Transfer::HttpManager::ERR_TYPE error, const boost::system::error_code& boost_error) {

        SILOG(transfer, debug, "multi_request_finished callback");

        if (error == Transfer::HttpManager::SUCCESS) {
            TS_ASSERT(response);
            if(response) {
                TS_ASSERT(response->getHeaders().size() != 0);
                std::map<std::string, std::string>::const_iterator it = response->getHeaders().find("Content-Length");
                TS_ASSERT(it != response->getHeaders().end());
                TS_ASSERT(response->getStatusCode() == 200);
                TS_ASSERT(response->getData());
                TS_ASSERT(response->getData()->length() == (uint64)response->getContentLength());
                TS_ASSERT(response->getContentLength() == 11650);
            }
        } else if (error == Transfer::HttpManager::REQUEST_PARSING_FAILED) {
            TS_FAIL("HTTP Request parsing failed");
        } else if (error == Transfer::HttpManager::RESPONSE_PARSING_FAILED) {
            TS_FAIL("HTTP Response parsing failed");
        } else if (error == Transfer::HttpManager::BOOST_ERROR) {
            TS_FAIL("HTTP request failed with a boost error: " + boost_error.message());
        } else {
            TS_FAIL("Got unknown response code from HttpManager");
        }

        boost::unique_lock<boost::mutex> lock(mNumCbsMutex);
        mNumCbs--;
        SILOG(transfer, debug, "Finished stress test file, still " << mNumCbs << " left");
        if(mNumCbs == 0) {
            mDone.notify_all();
        }
    }

    void request_finished(std::tr1::shared_ptr<Transfer::HttpManager::HttpResponse> response,
            Transfer::HttpManager::ERR_TYPE error, const boost::system::error_code& boost_error) {

        std::tr1::shared_ptr<Transfer::HttpManager::HttpResponse> bad;
        mHttpResponse = bad;

        if (error == Transfer::HttpManager::SUCCESS) {
            mHttpResponse = response;
        } else if (error == Transfer::HttpManager::REQUEST_PARSING_FAILED) {
            TS_FAIL("HTTP Request parsing failed");
        } else if (error == Transfer::HttpManager::RESPONSE_PARSING_FAILED) {
            TS_FAIL("HTTP Response parsing failed");
        } else if (error == Transfer::HttpManager::BOOST_ERROR) {
            TS_FAIL("HTTP request failed with a boost error: " + boost_error.message());
        } else {
            TS_FAIL("Got unknown response code from HttpManager");
        }

        mDone.notify_all();
    }

};

boost::condition_variable done;
boost::mutex mut;
int numClis = 0;

class RequestVerifier {
public:
    typedef std::tr1::function<void()> VerifyFinished;
    virtual void addToPool(std::tr1::shared_ptr<Transfer::TransferPool> pool,
            VerifyFinished cb, Transfer::TransferRequest::PriorityType priority) = 0;
    virtual ~RequestVerifier() {}
};

class MetadataVerifier
    : public RequestVerifier {
protected:
    uint64 mFileSize;
    Transfer::Fingerprint mHash;
    Transfer::URI mURI;
    std::tr1::shared_ptr<Transfer::RemoteFileMetadata> mMetadata;
    bool mGotResponse;

    void metadataFinished(std::tr1::shared_ptr<Transfer::MetadataRequest> request,
            std::tr1::shared_ptr<Transfer::RemoteFileMetadata> response, VerifyFinished cb) {
        SILOG(transfer, debug, "verifying metadata for " << request->getURI().toString());
        TS_ASSERT(response);
        TS_ASSERT(response->getSize() == mFileSize);
        TS_ASSERT(response->getFingerprint() == mHash);
        TS_ASSERT(response->getURI() == mURI);
        mMetadata = response;

        {
            boost::unique_lock<boost::mutex> lock(mut);
            mGotResponse = true;
        }
        cb();
    }

public:
    MetadataVerifier(Transfer::URI uri, uint64 file_size, const char * hash)
        : mFileSize(file_size), mHash(Transfer::Fingerprint::convertFromHex(hash)), mURI(uri), mGotResponse(false) {
    }
    void addToPool(std::tr1::shared_ptr<Transfer::TransferPool> pool,
            VerifyFinished cb, Transfer::TransferRequest::PriorityType priority) {
        std::tr1::shared_ptr<Transfer::TransferRequest> req(
                new Transfer::MetadataRequest(mURI, priority, std::tr1::bind(
                &MetadataVerifier::metadataFinished, this, std::tr1::placeholders::_1, std::tr1::placeholders::_2, cb)));
        pool->addRequest(req);

        if(mHash.toString() == "719c397d1019e56e41b5f98b1074abf32fb6e1fb832984f6d47a5761cfa3bcf6") {
            boost::unique_lock<boost::mutex> lock(mut);
            if(!mGotResponse) {
                pool->deleteRequest(req);
                numClis--;
            }
        }
    }
};


class ChunkVerifier
    : public MetadataVerifier {
private:
    uint64 mChunkSize;
    Transfer::Fingerprint mChunkHash;

public:
    ChunkVerifier(Transfer::URI uri, uint64 file_size, const char * file_hash,
            uint64 chunk_size, const char * chunk_hash)
        : MetadataVerifier(uri, file_size, file_hash), mChunkSize(chunk_size),
          mChunkHash(Transfer::Fingerprint::convertFromHex(chunk_hash)) {
	        }
    void addToPool(std::tr1::shared_ptr<Transfer::TransferPool> pool,
            VerifyFinished cb, Transfer::TransferRequest::PriorityType priority) {
        MetadataVerifier::addToPool(pool, std::tr1::bind(&ChunkVerifier::metadataFinished, this, pool, cb, priority), priority);
    }
    void metadataFinished(std::tr1::shared_ptr<Transfer::TransferPool> pool,
            VerifyFinished cb, Transfer::TransferRequest::PriorityType priority) {

        //Make sure chunk given is part of file
        SILOG(transfer, debug, "Verifying metadata response for chunk " << mURI.toString());
        std::tr1::shared_ptr<Transfer::Chunk> chunk;
        const Transfer::ChunkList & chunks = mMetadata->getChunkList();
        for (Transfer::ChunkList::const_iterator it = chunks.begin(); it != chunks.end(); it++) {
            if(it->getHash() == mChunkHash) {
                std::tr1::shared_ptr<Transfer::Chunk> found(new Transfer::Chunk(*it));
                chunk = found;
            }
        }
        TS_ASSERT(chunk);

        std::tr1::shared_ptr<Transfer::TransferRequest> req(
                new Transfer::ChunkRequest(mURI, *mMetadata, *chunk, priority, std::tr1::bind(
                &ChunkVerifier::chunkFinished, this, std::tr1::placeholders::_1, std::tr1::placeholders::_2, cb)));

        pool->addRequest(req);

        //invert the priority just to test updating priority
        pool->updatePriority(req, 1.0 - req->getPriority());
    }
    void chunkFinished(std::tr1::shared_ptr<Transfer::ChunkRequest> request,
            std::tr1::shared_ptr<const Transfer::DenseData> response, VerifyFinished cb) {
        SILOG(transfer, debug, "verifying chunk response for chunk " << mURI.toString());
        TS_ASSERT(request);
        TS_ASSERT(response);
        SILOG(transfer, debug, "response size = " << response->size() << ", req size = " << request->getChunk().getRange().size() );
        TS_ASSERT(response->size() == request->getChunk().getRange().size());
        TS_ASSERT(Transfer::Fingerprint::computeDigest(response->data(), response->length()) ==
                request->getChunk().getHash());
        cb();
    }
};

class SampleClient {

	Transfer::TransferMediator& mTransferMediator;
	std::tr1::shared_ptr<Transfer::TransferPool> mTransferPool;
	const std::string mClientID;
	std::vector<std::tr1::shared_ptr<RequestVerifier> > mReqList;

public:

	SampleClient(Transfer::TransferMediator& transferMediator, const std::string & clientID,
	        std::vector<std::tr1::shared_ptr<RequestVerifier> > reqList) :
		mTransferMediator(transferMediator), mClientID(clientID), mReqList(reqList) {
		boost::unique_lock<boost::mutex> lock(mut);
		numClis += reqList.size();
		SILOG(transfer, debug, "sample client started! increased numClis to " << numClis);
	}

	void run() {
		using std::tr1::placeholders::_1;

		//Register with the transfer mediator!
		mTransferPool = mTransferMediator.registerClient(mClientID);

        for(std::vector<std::tr1::shared_ptr<RequestVerifier> >::iterator it = mReqList.begin(); it != mReqList.end(); it++) {
            float pri = rand()/(float(RAND_MAX)+1);
            (*it)->addToPool(mTransferPool, std::tr1::bind(&SampleClient::request_finished, this), pri);
        }
	}

	void request_finished() {
        boost::unique_lock<boost::mutex> lock(mut);
        numClis--;
        SILOG(transfer, debug, mClientID << " request finished! reduced numClis to " << numClis);
        if(numClis <= 0) {
            done.notify_all();
        }
	}

};

class DiskManagerTest : public CxxTest::TestSuite {

public:

    std::string testData;
    std::tr1::shared_ptr<Transfer::DenseData> mData;

    DiskManagerTest()
         : CxxTest::TestSuite(),
           testData("BlahBlahMessage\r\nTestline2\nLineEndingsAreCool\n\n\n") {
    }

    void setUp() {

    }

    void tearDown() {

    }

    void scanCallback(std::tr1::shared_ptr<Transfer::DiskManager::ScanRequest::DirectoryListing> dirListing) {
        SILOG(transfer, debug, "Got directory scan callback!");

        TS_ASSERT(dirListing);
        if(dirListing) {
            TS_ASSERT(dirListing->size() > 0);
        }

        boost::unique_lock<boost::mutex> lock(mut);
        done.notify_all();
    }

    void testDirectoryScan() {
        SILOG(transfer, debug, "Testing reading root dir...");

        std::tr1::shared_ptr<Transfer::DiskManager::DiskRequest> req(
                new Transfer::DiskManager::ScanRequest("/",
                std::tr1::bind(&DiskManagerTest::scanCallback, this, std::tr1::placeholders::_1)));

        Transfer::DiskManager::getSingleton().addRequest(req);

        boost::unique_lock<boost::mutex> lock(mut);
        done.wait(lock);
    }

    void writeCallback(bool status) {
        SILOG(transfer, debug, "Got file write callback!");

        TS_ASSERT(status);

        boost::unique_lock<boost::mutex> lock(mut);
        done.notify_all();
    }

    void readCallback(std::tr1::shared_ptr<Transfer::DenseData> fileContents) {
        SILOG(transfer, debug, "Got file read callback!");

        TS_ASSERT(fileContents);
        if(fileContents) {
            std::string s = fileContents->asString();
            TS_ASSERT(s == testData);
        }

        boost::unique_lock<boost::mutex> lock(mut);
        done.notify_all();
    }

    void writeThenRead() {
        std::tr1::shared_ptr<Transfer::DenseData> toWrite(new Transfer::DenseData(testData));
        mData = toWrite;

        std::tr1::shared_ptr<Transfer::DiskManager::DiskRequest> req(
                new Transfer::DiskManager::WriteRequest("testFileWrite_TESTFILE.txt", mData,
                std::tr1::bind(&DiskManagerTest::writeCallback, this, std::tr1::placeholders::_1)));

        Transfer::DiskManager::getSingleton().addRequest(req);

        {
            boost::unique_lock<boost::mutex> lock(mut);
            done.wait(lock);
        }


        SILOG(transfer, debug, "Testing file read...");
        std::tr1::shared_ptr<Transfer::DiskManager::DiskRequest> req2(
                new Transfer::DiskManager::ReadRequest("testFileWrite_TESTFILE.txt",
                std::tr1::bind(&DiskManagerTest::readCallback, this, std::tr1::placeholders::_1)));

        Transfer::DiskManager::getSingleton().addRequest(req2);

        {
            boost::unique_lock<boost::mutex> lock(mut);
            done.wait(lock);
        }
    }

    void testFileWriteThenRead() {
        SILOG(transfer, debug, "Testing file write...");

        writeThenRead();

        SILOG(transfer, debug, "Increasing the size of test file from " << testData.length() << " bytes");
        for(int i=0; i<12; i++) {
            testData = testData + testData;
        }
        SILOG(transfer, debug, "New size of test file = " << testData.length());

        SILOG(transfer, debug, "Repeating write and read calls...");
        writeThenRead();
    }

};

class TransferTest : public CxxTest::TestSuite {

	//for ease of use
	typedef Transfer::URIContext URIContext;

	//Mediates transfers between subsystems (graphics, physics, etc)
	Transfer::TransferMediator* mTransferMediator;

	SampleClient* mSampleClient1;
	SampleClient* mSampleClient2;
	SampleClient* mSampleClient3;

	Thread* mClientThread1;
	Thread* mClientThread2;
	Thread* mClientThread3;
	ThreadSafeQueue<int> mTestQueue;

public:

	TransferTest()
         : CxxTest::TestSuite() {
            InitOptions(); // For CDN settings
            FakeParseOptions();
            mTransferMediator = &Transfer::TransferMediator::getSingleton();
	}

	void setUp() {
		//5 urls
		std::vector<std::tr1::shared_ptr<RequestVerifier> > list1;
		list1.push_back(std::tr1::shared_ptr<RequestVerifier>(new MetadataVerifier(
		        Transfer::URI("meerkat:///test/polySurface01.mesh"),
		        156125,
		        "85dbb7af4eed5c4e01eb64c70fc299946a2c19080173ea648948cca2039e30b4")));
        list1.push_back(std::tr1::shared_ptr<RequestVerifier>(new MetadataVerifier(
                Transfer::URI("meerkat:///test/blackclear.png"),
                167,
                "b18db4fb7971117be7124c7a05e2afb265fb26d0181f15f038a4628e2ae9b571")));
        list1.push_back(std::tr1::shared_ptr<RequestVerifier>(new MetadataVerifier(
                Transfer::URI("meerkat:///test/ArchD_Triumph_mesh.mesh"),
                215464,
                "e61a8cac4c938ea80182f8e23499c9c83248d01e1b506716bcca79b04cdaded5")));
        list1.push_back(std::tr1::shared_ptr<RequestVerifier>(new MetadataVerifier(
                Transfer::URI("meerkat:///test/whitecube_bk.png"),
                160,
                "2b4d4a27c51611238140e9980d4954ba56e8d90e7b73545fe46ce3f701cff034")));
        list1.push_back(std::tr1::shared_ptr<RequestVerifier>(new MetadataVerifier(
                Transfer::URI("meerkat:///test/arch.os"),
                3657,
                "719c397d1019e56e41b5f98b1074abf32fb6e1fb832984f6d47a5761cfa3bcf6")));

		//4 new urls, 1 overlap from list1
		std::vector<std::tr1::shared_ptr<RequestVerifier> > list2;
        list2.push_back(std::tr1::shared_ptr<RequestVerifier>(new MetadataVerifier(
                Transfer::URI("meerkat:///test/blackclear.png"),
                167,
                "b18db4fb7971117be7124c7a05e2afb265fb26d0181f15f038a4628e2ae9b571")));
        list2.push_back(std::tr1::shared_ptr<RequestVerifier>(new ChunkVerifier(
                        Transfer::URI("meerkat:///test/arcade.os"),
                        6246,
                        "58c9d20206a4ee3cf422b7595decf6edb2c2705a96ab09e0495a622b6bf5caea",
                        6246,
                        "58c9d20206a4ee3cf422b7595decf6edb2c2705a96ab09e0495a622b6bf5caea")));
        list2.push_back(std::tr1::shared_ptr<RequestVerifier>(new MetadataVerifier(
                Transfer::URI("meerkat:///test/whitecube_up.png"),
                160,
                "2b4d4a27c51611238140e9980d4954ba56e8d90e7b73545fe46ce3f701cff034")));
        list2.push_back(std::tr1::shared_ptr<RequestVerifier>(new MetadataVerifier(
                Transfer::URI("meerkat:///test/whitecube_rt.png"),
                160,
                "2b4d4a27c51611238140e9980d4954ba56e8d90e7b73545fe46ce3f701cff034")));

		//3 new urls, 1 overlap from list1, 1 overlap from list2
		std::vector<std::tr1::shared_ptr<RequestVerifier> > list3;
        list3.push_back(std::tr1::shared_ptr<RequestVerifier>(new MetadataVerifier(
                Transfer::URI("meerkat:///test/polySurface01.mesh"),
                156125,
                "85dbb7af4eed5c4e01eb64c70fc299946a2c19080173ea648948cca2039e30b4")));
		list3.push_back(std::tr1::shared_ptr<RequestVerifier>(new ChunkVerifier(
		                Transfer::URI("meerkat:///test/arcade.os"),
		                6246,
		                "58c9d20206a4ee3cf422b7595decf6edb2c2705a96ab09e0495a622b6bf5caea",
		                6246,
		                "58c9d20206a4ee3cf422b7595decf6edb2c2705a96ab09e0495a622b6bf5caea")));
        list3.push_back(std::tr1::shared_ptr<RequestVerifier>(new MetadataVerifier(
                Transfer::URI("meerkat:///test/blackcube_rt.png"),
                158,
                "784a5b0809ed274036733777f9a158678ffa852247deefd4b1140409dffdac14")));

		mSampleClient1 = new SampleClient(*mTransferMediator, "sample1", list1);
		mSampleClient2 = new SampleClient(*mTransferMediator, "sample2", list2);
		mSampleClient3 = new SampleClient(*mTransferMediator, "sample3", list3);

		mClientThread1 = new Thread(std::tr1::bind(&SampleClient::run, mSampleClient1));
		mClientThread2 = new Thread(std::tr1::bind(&SampleClient::run, mSampleClient2));
		mClientThread3 = new Thread(std::tr1::bind(&SampleClient::run, mSampleClient3));
	}

	void tearDown() {
        //Make sure clients have exited
	    mClientThread1->join();
        mClientThread2->join();
        mClientThread3->join();

        //Wait for transfer mediator thread to exit
        mTransferMediator->cleanup();
	}

	void testTransferRequests() {
		srand ( time(NULL) );
		boost::unique_lock<boost::mutex> lock(mut);
		done.wait(lock);
	}

};

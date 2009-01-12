/*     Iridium ransfer -- Content Transfer management system
 *  HTTPTransfer.cpp
 *
 *  Copyright (c) 2008, Patrick Reiter Horn
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
 *  * Neither the name of Iridium nor the names of its contributors may
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
/*  Created on: Dec 31, 2008 */
#include "HTTPRequest.hpp"
#include "TransferData.hpp"
#include "util/ThreadSafeQueue.hpp"

#include <boost/thread.hpp>

#include <curl/curl.h>

namespace Iridium {
namespace Transfer {


size_t HTTPRequest::write_cb(unsigned char *data, size_t length, size_t count, HTTPRequest *handle) {
	return handle->write(data, length*count);
}

size_t HTTPRequest::read_cb(unsigned char *data, size_t length, size_t count, HTTPRequest *handle) {
	return handle->read(data, length*count);
}

static curl_off_t HTTPRequest_seek_cb(HTTPRequest *handle, curl_off_t offset, int origin) {
	switch (origin) {
	case SEEK_SET:
		handle->seek(handle->getData()->startbyte() + offset);
		break;
	case SEEK_CUR:
		handle->seek(handle->getOffset() + offset);
		break;
	case SEEK_END:
		handle->seek(handle->getData()->endbyte() + offset);
		break;
	}
	return handle->getOffset();
}

size_t HTTPRequest::header_cb(char *data, size_t length, size_t count, HTTPRequest *handle) {
	handle->gotHeader(std::string((unsigned char*)data, (unsigned char*)data + length*count));
	return length*count;
}

size_t HTTPRequest::write(const unsigned char *copyFrom, size_t length) {
	if (mOffset < mData->startbyte()) {
		return 0;
	}
	if (mData->length() < (size_t)(mOffset - mData->startbyte()) + length) {
		mData->setLength((mOffset - mData->startbyte()) + length);
	}
	unsigned char *copyTo = &(mData->mData[mOffset]);
	std::copy(copyFrom, copyFrom + length, copyTo);
	mOffset += length;
	return length;
}

size_t HTTPRequest::read(unsigned char *copyTo, size_t length) {
	if (mOffset < mData->startbyte()) {
		return 0;
	}
	if (mData->length() < (size_t)(mOffset - mData->startbyte()) + length) {
		length = mData->length() - (size_t)(mOffset - mData->startbyte());
	}
	unsigned char *copyFrom = &(mData->mData[mOffset]);
	std::copy(copyFrom, copyFrom + length, copyTo);
	mOffset += length;
	return length;
}

void HTTPRequest::gotHeader(const std::string &header) {
	std::string::size_type colon = header.find(':');
	if (colon == std::string::npos) {
		// either "HTTP/1.1 200 OK" or else a malformed header.
		return;
	}
	std::string headername = header.substr(0, colon);
	std::string headervalue = header.substr(colon+1);
	for (std::string::size_type i = 0; i < colon; i++) {
		headername[i] = tolower(headername[i]);
	}
	if (headername == "content-length") {
		size_t dataToReserve = 0;
		// lexical_cast doesn't correctly handle whitespace.
		if (sscanf(headervalue.c_str(), "%lu", &dataToReserve) == 1) {
			mData->setLength(dataToReserve);
			std::cout << "LENGTH IS NOW "<< dataToReserve << std::endl;
		}
	}
	std::cout << "Got header [" << headername << "] = " << headervalue << std::endl;
}


#define RETRY_TIME 1.0

namespace {

	static boost::once_flag flag = BOOST_ONCE_INIT;

	//static ThreadSafeQueue<HTTPRequest*> requestQueue;
	boost::mutex http_lock;

	boost::thread main_loop;

	CURLM *curlm;
	CURL *parent_easy_curl;

}
extern "C"
void HTTPRequest::destroyCurl () {
	curl_multi_cleanup(curlm);
	curl_global_cleanup();
}

void HTTPRequest::initCurl () {
	/*
	 * FIXME: don't use CURL_WIN32_INIT as we use sockets for things other than curl.
	 */
	/*
	 * FIXME: don't run curl_global_init here! The manpage says it *SHOULD* be run when
	 * only one thread is executing in the entire program (main() is best) as it is
	 * not thread-safe with a lot of libraries!
	 */
	curl_global_init(CURL_GLOBAL_ALL);
	curlm = curl_multi_init();
	atexit(&destroyCurl);

	curl_multi_setopt(curlm, CURLMOPT_PIPELINING, 1);
	curl_multi_setopt(curlm, CURLMOPT_MAXCONNECTS, 8); // make higher if a server.

	// CURLOPT_PROGRESSFUNCTION may be useful for determining whether to timeout during an active connection.
	parent_easy_curl = curl_easy_init( );
	curl_easy_setopt(parent_easy_curl, CURLOPT_VERBOSE, 1);
	curl_easy_setopt(parent_easy_curl, CURLOPT_NOPROGRESS, 1);
	curl_easy_setopt(parent_easy_curl, CURLOPT_NOSIGNAL, 1);
	curl_easy_setopt(parent_easy_curl, CURLOPT_WRITEFUNCTION, &HTTPRequest::write_cb);
	curl_easy_setopt(parent_easy_curl, CURLOPT_READFUNCTION, &HTTPRequest::read_cb);
	curl_easy_setopt(parent_easy_curl, CURLOPT_SEEKFUNCTION, &HTTPRequest_seek_cb);
	curl_easy_setopt(parent_easy_curl, CURLOPT_HEADERFUNCTION, &HTTPRequest::header_cb);
	curl_easy_setopt(parent_easy_curl, CURLOPT_VERBOSE, 1);
	curl_easy_setopt(parent_easy_curl, CURLOPT_FAILONERROR, 1);
	curl_easy_setopt(parent_easy_curl, CURLOPT_USERAGENT, "Iridium/0.1 (" __DATE__ ")");
	curl_easy_setopt(parent_easy_curl, CURLOPT_CONNECTTIMEOUT, 5);
	// curl_easy_setopt(parent_easy_curl, CURLOPT_TIMEOUT, ...); // if the connection is tarpitted by a nasty firewall...

	// CURLOPT_DNS_USE_GLOBAL_CACHE: WARNING: this option is considered obsolete. Stop using it. Switch over to using the share interface instead! See CURLOPT_SHARE and curl_share_init(3).
	// From curl_multi_add_handle: If the easy handle is not set to use a shared (CURLOPT_SHARE) or global DNS cache (CURLOPT_DNS_USE_GLOBAL_CACHE), it will be made to use the DNS cache that is shared between all easy handles within the multi handle when curl_multi_add_handle(3) is called.

	/*
	if (OPTIONS->proxydata) {
		//CURLPROXY_HTTP, CURLPROXY_SOCKS4 (added in 7.15.2), CURLPROXY_SOCKS5, CURLPROXY_SOCKS4A (added in 7.18.0) and CURLPROXY_SOCKS5_HOSTNAME
		curl_easy_setopt(parent_easy_curl, CURLOPT_PROXY, OPTIONS->proxydata);
		curl_easy_setopt(parent_easy_curl, CURLOPT_PROXYTYPE, OPTIONS->proxytype);
		// CURLOPT_PROXYUSERNAME
		// CURLOPT_PROXYPASSWORD
		// CURLOPT_PROXYAUTH
	}
	*/

	main_loop = boost::thread(&curlLoop);

}

/*
ThreadSafeQueue shasumQueue;
void shasumLoop() {
	HTTPRequest *req;
	while (true) {
		req = shasumQueue.blockingPop();
		computeShasum(req->mData);
		...
	}
}
*/

void HTTPRequest::curlLoop () {
	while (true) {
		int numevents;

		while (true) {
			boost::lock_guard<boost::mutex> access_curl_handle(http_lock);
			CURLMsg *transferMsg = curl_multi_info_read(curlm, &numevents);
			if (transferMsg == NULL) {
				break;
			}
			CURL *handle = transferMsg->easy_handle;
			void *dataptr;
			curl_easy_getinfo(handle, CURLINFO_PRIVATE, &dataptr);
			HTTPRequest *request = (HTTPRequest*)dataptr;

			if (transferMsg->msg == CURLMSG_DONE) {
				bool success;
				if (transferMsg->data.result == 0) {
					success = true;
				} else {
					// CURLE_RANGE_ERROR
					// CURLE_HTTP_RETURNED_ERROR
					std::cout << "failed " << request << ": " <<
							curl_easy_strerror(transferMsg->data.result) << std::endl;
					success = false;
				}
				curl_easy_cleanup(handle);
				request->mCallback(request, request->getData(), success);
				request->mCallback = NULL;
				request->mCurlRequest = NULL;
			}
		}

		fd_set read_fd_set, write_fd_set, exc_fd_set;
		long timeout_ms;
		FD_ZERO(&read_fd_set);
		FD_ZERO(&write_fd_set);
		FD_ZERO(&exc_fd_set);
		int max_fd;
		curl_multi_fdset(curlm,
					&read_fd_set,
					&write_fd_set,
					&exc_fd_set,
					&max_fd);
		curl_multi_timeout(curlm, &timeout_ms);
		if (max_fd >= 0 && timeout_ms >= 0) {
			struct timeval timeout_tv;
			timeout_tv.tv_usec = 1000*(timeout_ms%1000);
			timeout_tv.tv_sec = timeout_ms/1000;
			select(max_fd+1, &read_fd_set, &write_fd_set, &exc_fd_set, &timeout_tv);
		}

		{
			boost::lock_guard<boost::mutex> access_curl_handle(http_lock);
			while (curl_multi_perform(curlm, &numevents) == CURLM_CALL_MULTI_PERFORM) {
				// do nothing...
			}
		}

	}
}

void HTTPRequest::abort() {
	boost::lock_guard<boost::mutex> access_curl_handle(http_lock);

	CURL *handle = mCurlRequest;
	if (handle) {
		curl_easy_cleanup(handle);
	}
	if (mCallback) {
		mCallback(this, getData(), false); // should delete this.
	}
}

void HTTPRequest::go() {
	boost::lock_guard<boost::mutex> curl_lock (http_lock);

	boost::call_once(&initCurl, flag);

	mCurlRequest = curl_easy_duphandle(parent_easy_curl);
	curl_easy_setopt(mCurlRequest, CURLOPT_WRITEDATA, this);
	curl_easy_setopt(mCurlRequest, CURLOPT_READDATA, this);
	curl_easy_setopt(mCurlRequest, CURLOPT_SEEKDATA, this);
	curl_easy_setopt(mCurlRequest, CURLOPT_HEADERDATA, this);
	curl_easy_setopt(mCurlRequest, CURLOPT_PRIVATE, this);
	// guaranteed to reain valid until the next non-const member function:
	curl_easy_setopt(mCurlRequest, CURLOPT_URL, this->mURI.uri().c_str());

	curl_multi_add_handle(curlm, mCurlRequest);

	//requestQueue.push(this);
	// CURLOPT_REFERER
}


//Task::timer_queue.schedule(Task::AbsTime::now() + RETRY_TIME,
//	boost::bind(&getData, this, fileId, requestedRange, callback, triesLeft-1));

}
}

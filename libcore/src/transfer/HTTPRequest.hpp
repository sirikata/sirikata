/*  Sirikata Network Utilities
 *  HTTPRequest.hpp
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
/*  Created on: Jan 7, 2009 */

#ifndef SIRIKATA_HTTPRequest_HPP__
#define SIRIKATA_HTTPRequest_HPP__

#include <vector>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/utility.hpp>
#include "URI.hpp"
#include "TransferData.hpp"

namespace Sirikata {
namespace Transfer {

typedef void CURL;

/// Downloads the specified file in another thread and calls callback when finished.
class HTTPRequest {
public:
	typedef boost::function3<void, HTTPRequest*,
			const DenseDataPtr &, bool> CallbackFunc;
private:
	const URI mURI; // const because its c_str is passed to curl.
	Range mRequestedRange;
	CallbackFunc mCallback;
	CURL *mCurlRequest;

	Range::base_type mOffset;
	DenseDataPtr mData;

	/** The default callback--useful for POST queries where you do not care about the response */
	static void nullCallback(HTTPRequest*, const DenseDataPtr &, bool){
	}

	size_t write(const unsigned char *begin, size_t amount);
	size_t read(unsigned char *begin, size_t amount);
	void gotHeader(const std::string &header);

	static void curlLoop();
	static void initCurl();
	static void destroyCurl();
	static CURL *allocDefaultCurl();

	static size_t write_cb(unsigned char *data, size_t length, size_t count, HTTPRequest *handle);
	static size_t read_cb(unsigned char *data, size_t length, size_t count, HTTPRequest *handle);
	static size_t header_cb(char *data, size_t length, size_t count, HTTPRequest *handle);

public:

	inline HTTPRequest(const HTTPRequest &other)
		: mURI(other.mURI), mRequestedRange(other.mRequestedRange),
		mCallback(other.mCallback), mCurlRequest(NULL),
		mOffset(other.mOffset), mData(other.mData){
		// can't copy after starting the curl request.
		assert(!other.mCurlRequest);
	}

	inline const DenseDataPtr &getData() {
		return mData;
	}

	inline Range::base_type getOffset() const {
		return mOffset;
	}

	inline void seek(Range::base_type offset) {
		if (offset >= mRequestedRange.startbyte() &&
				offset < mRequestedRange.endbyte()) {
			mOffset = offset;
		}
	}

	HTTPRequest(const URI &uri, const Range &range)
		: mURI(uri), mRequestedRange(range), mCallback(&nullCallback),
		mCurlRequest(NULL), mOffset(0), mData(new DenseData(range)) {
	}

	~HTTPRequest() {
		if (mCurlRequest) {
			abort();
		}
	}

	/// URI getter
	inline const URI &getURI() const {return mURI;}

	/// Range getter
	inline const Range &getRange() const {return mRequestedRange;}

	void abort();

	/// Setter for the response function.
	inline void setCallback(const CallbackFunc &cb) {
		mCallback = cb;
	}

	/**
	 *  Executes the query.  The 'this' pointer must be on the heap and
	 *  must not be subsequently referenced.  It will be placed in this
	 *  thread's work queue.
	 */

	void go();
};

//typedef boost::shared_ptr<HTTPRequest> HTTPRequestPtr;

}
}

#endif /* SIRIKATA_HTTPRequest_HPP__ */

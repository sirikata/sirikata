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

#include "URI.hpp"
#include "TransferData.hpp"

extern "C" struct curl_httppost;

namespace Sirikata {
namespace Transfer {


typedef void CURL;

typedef std::tr1::shared_ptr<class HTTPRequest> HTTPRequestPtr;

/// Downloads the specified file in another thread and calls callback when finished.
class SIRIKATA_EXPORT HTTPRequest {
public:

	typedef std::tr1::function<void(HTTPRequest*,
			const DenseDataPtr &, bool)> CallbackFunc;
private:
	HTTPRequestPtr mPreventDeletion; ///< set to shared_from_this while cURL owns a reference.

	enum {NEW, INPROGRESS, FINISHED} mState;

	const URI mURI;
	std::string mURIString;
	std::string mRangeString;
	std::vector<std::string> mHeaderStorage;
	
	Range mRequestedRange;
	CallbackFunc mCallback;
	CURL *mCurlRequest;
	void *mHeaders; // CURL header linked list.

	/// Request types:
	curl_httppost *mCurlFormBegin; ///< POST multipart/form-data
	curl_httppost *mCurlFormEnd;
	std::string mSimplePOSTString; ///< POST application/x-www-form-urlencoded
	bool mTypeDELETE; ///< DELETE
	long mStatusCode;

	Range::base_type mUploadOffset;
	DenseDataPtr mStreamUploadData; ///< PUT or ftp upload

	std::vector<DenseDataPtr> mDataReferences; ///< prevent from being freed.

	Range::base_type mOffset;
	Range::length_type mFullFilesizeOnServer;
	MutableDenseDataPtr mData;

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

	void setFinalProperties(); ///< Will be called if the request must be retried.
	void initCurlHandle(); ///< Only called initially--sets defaults for all properties.
	void finalGo(); ///< Actually performs the request, if not too many connections are active.

	HTTPRequest(const HTTPRequest &other);
public:

	/** Do not ever use this--it is not thread safe, and the DenseDataPtr
	 is passed to the callback anyway. May be made private */
	inline const MutableDenseDataPtr &getData() {
		return mData;
	}

	/// Returns the DenseData passed to setPUTData()
	inline const DenseDataPtr &getUploadData() {
		return mStreamUploadData;
	}

	inline Range::base_type getUploadOffset() const {
		return mUploadOffset;
	}

	/** Gets the current transfer offset--useful for a progress indicator.
	 * You can pair this up with getRange().endbyte() to get a percentage.
	 *
	 * @returns the last (absolute) byte written;
	 *          subtract getData().startbyte() to get the relative byte.
	 */
	inline Range::base_type getOffset() const {
		return mOffset;
	}

	/** Changes the location in the DenseData that we write
	 * to when we get data. Do not call this except from a callback
	 * when streaming. In most cases it will be unnecessary,
	 * as the denseData should be cleared after each packet.
	 */
	inline void seek(Range::base_type offset) {
		if (offset < mStreamUploadData->length()) {
			mUploadOffset = offset;
		} else {
			mUploadOffset = mStreamUploadData->length();
		}
	}

	HTTPRequest(const URI &uri, const Range &range)
		: mState(NEW),
		  mURI(uri), mRequestedRange(range), mCallback(&nullCallback),
		  mCurlRequest(NULL), mHeaders(NULL),
		  mCurlFormBegin(NULL), mCurlFormEnd(NULL), mTypeDELETE(false)
		  {
		initCurlHandle();
	}

	~HTTPRequest();

	/// URI getter
	inline const URI &getURI() const {return mURI;}

	/// Range getter
	inline const Range &getRange() const {return mRequestedRange;}

	/** Returns the length of the file as it exists on the server,
	 even if the request we sent was a HEAD or contained a Range. 
	This function may return 0, indicating that the full length is unknown. */
	inline Range::length_type getFullLength() const { return mFullFilesizeOnServer; }

	/** Aborts an active transfer. Necessary for some streaming applications.
	 * Acts as if the transfer failed--calls callback and removes the
	 * self-reference so this class can be free'd.
	 */
	void abort();

	/// Setter for the response function.
	inline void setCallback(const CallbackFunc &cb) {
		mCallback = cb;
	}

	/**
	 * Deletes the request file. Retrieved data may be empty,
	 * but success should be true if the deletion was successful.
	 * Should work for HTTP(s), FTP and SFTP
	 *
	 * Use this with a REST-style upload system.
	 */
	void setDELETE();

	/**
	 * Performs an upload of this file. Again, there may be no retrieved data,
	 * but success should be true if the upload was successful.
	 * Should work for HTTP(s), FTP and SFTP.
	 *
	 * Use this with a REST-style upload system.
	 *
	 * @param uploadData  A SparseData that represents the file.
	 *                    Any non-contiguous chunks will be filled with 0's.
	 */
	void setPUTData(const DenseDataPtr &uploadData);

	/**
	 * Performs an upload of a set of files using HTTP/POST.
	 *
	 * This may be used in conjunction with addPOSTField if other
	 * non-file form fields are to be added.
	 *
	 * Currently, the Content-Type is hardcoded as application/octet-stream.
	 *
	 * @param fieldname  The name of the type="file" form field.
	 *                   Note: The form field should end with "[]"
	 * @param uploadData  A SparseData that represents the file.
	 *                    Any non-contiguous chunks will be filled with 0's.
	 * @param filename   The name of the file to be uploaded.
	 */
	void addPOSTArray(const std::string &fieldname,
			const std::vector<std::pair<std::string, DenseDataPtr> > &uploadData,
			const char *contentType="application/octet-stream");

	/**
	 * Performs an upload of this file using HTTP/POST.
	 *
	 * This may be used in conjunction with addPOSTField if other
	 * non-file form fields are to be added.
	 *
	 * @param fieldname  The name of the type="file" form field.
	 * @param filename   The name of the file to be uploaded.
	 * @param uploadData  A SparseData that represents the file.
	 *                    Any non-contiguous chunks will be filled with 0's.
	 */
	void addPOSTData(const std::string &fieldname,
			const std::string &filename,
			const DenseDataPtr &uploadData,
			const char *contentType="application/octet-stream");

	/**
	 * Creates a POST request using the multipart/form-data enctype.
	 * This may be used in conjunction with addPOSTData, or without.
	 *
	 * @param name  The name of this form field.
	 * @param value The value of this form field.
	 */
	void addPOSTField(const std::string &name, const std::string &value);

	/**
	 * Creates a POST request using the application/x-www-form-urlencoded enctype.
	 * This may not be used in conjunction with addPOSTData or addPOSTField
	 *
	 * Use this when request size is a concern, or when you are not
	 * uploading files.
	 *
	 * @param name  The name of this form field.
	 * @param value The value of this form field.
	 */
	void addSimplePOSTField(const std::string &name, const std::string &value);

	/**
	 * Creates a POST request using the application/x-www-form-urlencoded enctype.
	 * This may not be used in conjunction with addPOSTData or addPOSTField
	 *
	 * @param postString  The url-encoded form data.
	 */
	void setSimplePOSTString(const std::string &postString);

	/** Note: you are responsible for checking the protocol
	 * and using HTTP headers in the case of http, and valid FTP
	 * commands in the case of FTP. Not doing so may cause a security bug.
	 */
	void addHeader(const std::string &header);

	/**
	 *  Executes the query.
	 *
	 *  @param holdreference If this object was constructed with a shared_ptr,
	 *      which it should have been, pass that pointer into holdReference.
	 *      If you intend to manage memory yourself (this is dangerous), it
	 *      is wise to pass NULL here.
	 */
	void go(const HTTPRequestPtr &holdReference);
};

}
}

#endif /* SIRIKATA_HTTPRequest_HPP__ */

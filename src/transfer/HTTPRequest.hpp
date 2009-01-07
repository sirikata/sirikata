/*     Iridium Network Utilities
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
/*  Created on: Jan 7, 2009 */

#ifndef IRIDIUM_HTTPRequest_HPP__
#define IRIDIUM_HTTPRequest_HPP__

#include <vector>
#include <boost/function.hpp>
#include <boost/bind.hpp>

#include "URI.hpp"
#include "TransferData.hpp"

namespace Iridium {
namespace Transfer {

/// Downloads the specified file in another thread and calls callback when finished.
class HTTPRequest {
public:
	typedef boost::function3<void, HTTPRequest*,
			const DenseDataPtr &, bool> CallbackFunc;
private:
	URI mURI;
	Range mRange;
	CallbackFunc mCallback;

	/** The default callback--useful for POST queries where you do not care about the response */
	static void nullCallback(HTTPRequest*, const DenseDataPtr &, bool){
	}
public:

	HTTPRequest(const URI &uri, const Range &range)
		: mURI(uri), mRange(range), mCallback(&nullCallback) {
	}

	/// URI getter
	inline const URI &getURI() const {return mURI;}

	/// Range getter
	inline const Range &getRange() const {return mRange;}

	/// Setter for the response function.
	void setCallback(const CallbackFunc &cb) {
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

#endif /* IRIDIUM_HTTPRequest_HPP__ */

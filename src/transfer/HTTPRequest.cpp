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
#include "task/Time.hpp"
#include "task/TimerQueue.hpp"
#include "util/ThreadSafeQueue.hpp"
#include "DiskCache.hpp"
#include "MemoryCache.hpp"
#include "LRUPolicy.hpp"

namespace Iridium {
namespace Transfer {

#define RETRY_TIME 1.0

namespace {
static boost::once_flag flag = BOOST_ONCE_INIT;

static ThreadSafeQueue<HTTPRequest*> requestQueue;


void initHTTP () {
	DiskCache(new LRUPolicy(16384),NULL,NULL,NULL);
}

}



void HTTPRequest::go() {
	requestQueue.push(this);
	boost::call_once(initHTTP, flag);
}


//Task::timer_queue.schedule(Task::AbsTime::now() + RETRY_TIME,
//	boost::bind(&getData, this, fileId, requestedRange, callback, triesLeft-1));

}
}

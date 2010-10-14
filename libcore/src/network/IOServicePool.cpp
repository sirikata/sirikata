/*  Sirikata Network Utilities
 *  IOServicePool.cpp
 *
 *  Copyright (c) 2009, Daniel Reiter Horn
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

#include <sirikata/core/util/Standard.hh>
#include <sirikata/core/network/IOServicePool.hpp>
#include <sirikata/core/network/IOService.hpp>
#include <sirikata/core/network/IOServiceFactory.hpp>
#include <sirikata/core/network/IOWork.hpp>
#include <sirikata/core/util/Thread.hpp>

namespace Sirikata {
namespace Network {

IOServicePool::IOServicePool(uint32 nthreads)
 : mIO(IOServiceFactory::makeIOService()),
   mThreads(nthreads, NULL),
   mWork(NULL)
{
}

IOServicePool::~IOServicePool() {
    if (mWork) stopWork();
    for(ThreadList::iterator it = mThreads.begin(); it != mThreads.end(); it++)
        delete *it;
    IOServiceFactory::destroyIOService(mIO);
}

namespace {
void runWrapper(IOService* ios) {
    ios->run();
}
}
void IOServicePool::reset() {
    mIO->reset();
}
void IOServicePool::run() {
    for(ThreadList::iterator it = mThreads.begin(); it != mThreads.end(); it++)
        (*it) = new Thread( std::tr1::bind(runWrapper, mIO) );
}

void IOServicePool::join() {
    // Other threads won't work if they still have work
    stopWork();

    for(ThreadList::iterator it = mThreads.begin(); it != mThreads.end(); it++)
        (*it)->join();
}

void IOServicePool::startWork() {
    if (mWork) return;
    mWork = new IOWork(*mIO);
}

void IOServicePool::stopWork() {
    if (!mWork) return;

    delete mWork;
    mWork = NULL;
}


IOService* IOServicePool::service() {
    return mIO;
}

} // namespace Network
} // namespace Sirikata

/*  Sirikata Utilities -- Plugin
 *  Plugin.cpp
 *
 *  Copyright (c) 2009, Ewen Cheslack-Postava
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
#include "util/Standard.hh"
#include "util/Plugin.hpp"
#include <cassert>

namespace Sirikata {

Plugin::Plugin(const String& path)
 : mDL(path),
   mInit(NULL),
   mDestroy(NULL),
   mName(NULL),
   mRefCount(NULL),
   mInitialized(0)
{
}

Plugin::~Plugin() {
    // Destroy and unload just in case.
    while(mDestroy && mInitialized)
        destroy();
}

bool Plugin::load() {
    std::cout << "dbm: Plugin::load" << std::endl;
    if (!mDL.load()) {
        std::cout << "dbm: Plugin::load FAIL" << std::endl;
        return false;
    }
    std::cout << "dbm: Plugin::load SUCCESS" << std::endl;

    mInit = (InitFunc)mDL.symbol("init");
    mDestroy = (DestroyFunc)mDL.symbol("destroy");
    mName = (NameFunc)mDL.symbol("name");
    mRefCount = (RefCountFunc)mDL.symbol("refcount");
///    std::cout << std::hex << (unsigned long)mName << std::endl;
    std::cout << "dbm: Plugin::load RETURN: " 
            << std::hex << (unsigned long)mInit <<"|" 
            << std::hex << (unsigned long)mDestroy <<"|"
            << std::hex << (unsigned long)mName <<"|"
            << std::hex << (unsigned long)mRefCount 
            << std::endl;
    return (mInit != NULL && mDestroy != NULL && mName != NULL && mRefCount != NULL);
}

bool Plugin::unload() {
    mInit = NULL;
    mDestroy = NULL;
    mName = NULL;
    mRefCount = NULL;
    return mDL.unload();
}

void Plugin::initialize() {
    assert(mInit);
    std::cout << "dbm: Plugin::initialize about to call mInit" << std::endl;
    mInit();
    std::cout << "dbm: Plugin::initialize done" << std::endl;
    mInitialized++;
}

void Plugin::destroy() {
    assert(mDestroy && mInitialized > 0);
    mDestroy();
    mInitialized--;
}

String Plugin::name() {
    assert(mName);

    const char* n = mName();
    assert(n != NULL);

    return String(n);
}

int Plugin::refcount() {
    assert(mRefCount);
    return mRefCount();
}

} // namespace Sirikata

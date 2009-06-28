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
    if (!mDL.load())
        return false;

    mInit = (InitFunc)mDL.symbol("init");
    mDestroy = (DestroyFunc)mDL.symbol("destroy");
    mName = (NameFunc)mDL.symbol("name");
    mRefCount = (RefCountFunc)mDL.symbol("refcount");

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
    mInit();
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

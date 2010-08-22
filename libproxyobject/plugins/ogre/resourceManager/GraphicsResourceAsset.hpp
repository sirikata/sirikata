/*  Meru
 *  GraphicsResourceAsset.hpp
 *
 *  Copyright (c) 2009, Stanford University
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
#ifndef _GRAPHICS_RESOURCE_ASSET_HPP_
#define _GRAPHICS_RESOURCE_ASSET_HPP_

#include "GraphicsResource.hpp"
#include "ResourceDownloadTask.hpp"
#include "ResourceDependencyTask.hpp"
#include "ResourceLoadTask.hpp"
#include "ResourceUnloadTask.hpp"
#include "../meruCompat/MeruDefs.hpp"

namespace Meru {

class GraphicsResourceAsset : public GraphicsResource
{
public:
  GraphicsResourceAsset(const URI &uri, GraphicsResource::Type type);
  virtual ~GraphicsResourceAsset();

    /*inline const RemoteFileId &getRemoteFileId() const {
    return mResourceID;
    }*/

  inline const URI& getURI() const{
    return mURI;
  }
/*
  inline const SHA256 &getHash() const {
    return mResourceID.fingerprint();
    }*/

protected:
  virtual void doParse();
  virtual void doLoad();
  virtual void doUnload();

  virtual void parsed(bool success);
  virtual void loaded(bool success, unsigned int epoch);
  virtual void unloaded(bool success, unsigned int epoch);

  virtual DependencyTask * createDownloadTask(DependencyManager *manager, ResourceRequestor *resourceRequestor) = 0;
  virtual ResourceDependencyTask * createDependencyTask(DependencyManager *manager) = 0;
  virtual ResourceLoadTask * createLoadTask(DependencyManager *manager) = 0;
  virtual ResourceUnloadTask * createUnloadTask(DependencyManager *manager) = 0;

    //const RemoteFileId mResourceID;
    const URI mURI;

  ResourceLoadTask* mLoadTask;
  ResourceDependencyTask* mParseTask;
  ResourceUnloadTask* mUnloadTask;
};

}

#endif

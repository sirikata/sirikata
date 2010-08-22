/*  Meru
 *  GraphicsResourceName.cpp
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
#include "GraphicsResourceManager.hpp"
#include "GraphicsResourceAsset.hpp"
#include "GraphicsResourceName.hpp"
#include "ResourceManager.hpp"
#include <boost/bind.hpp>
#include <stdio.h>

using namespace std;
namespace Meru {

GraphicsResourceName::GraphicsResourceName(const URI &resourceID, GraphicsResource::Type referencedType)
: GraphicsResource(resourceID.toString(), NAME), mURI(resourceID), mReferencedType(referencedType)
{

}

GraphicsResourceName::~GraphicsResourceName()
{

}

SharedResourcePtr GraphicsResourceName::getReference()
{
  return *(mDependencies.begin());
}

void GraphicsResourceName::doParse()
{
    /*ResourceHash result;
  std::tr1::function<void(const URI &, const ResourceHash *)> callback = std::tr1::bind(&GraphicsResourceName::hashLookupCallback, getWeakPtr(), mReferencedType, _1, _2);

  if (ResourceManager::getSingleton().nameLookup(mURI, result, callback)) {
    hashLookupCallback(getWeakPtr(), mReferencedType, mURI, &result);
    }*/
    SharedResourcePtr resource = getWeakPtr().lock();
    if (resource) {
        try{
            GraphicsResourceManager *grm = GraphicsResourceManager::getSingletonPtr();
            SharedResourcePtr newResource = grm->getResourceAsset(mURI, mReferencedType);
            resource->addDependency(newResource);
            resource->parsed(true);
        }
        catch(std::invalid_argument &exc) {
            resource->parsed(false);
        }
    } else resource->parsed(false);
}

void GraphicsResourceName::doLoad()
{
  loaded(true, mLoadEpoch);
}

void GraphicsResourceName::doUnload()
{
  unloaded(true, mLoadEpoch);
}

// old name download callback, no longer used since TransferMediator does not
// deal with hashes

/*
void GraphicsResourceName::hashLookupCallback(WeakResourcePtr resourcePtr, Type refType, const URI &id, const ResourceHash *hash)
{
//  assert(id != hash);

  // add dependency for hash


  SharedResourcePtr resource = resourcePtr.lock();
  if (resource) {
    if (hash) {
      try {
        GraphicsResourceManager *grm = GraphicsResourceManager::getSingletonPtr();
        SharedResourcePtr hashResource = grm->getResourceAssetByHash(*hash, refType);
        resource->addDependency(hashResource);
        resource->parsed(true);
      }
      catch (std::invalid_argument &exc) {
        resource->parsed(false);
      }
    } else {
      resource->parsed(false);
   }
  }
}*/

void GraphicsResourceName::fullyParsed()
{
  assert(mDependencies.size() > 0);

  std::set<WeakResourcePtr>::iterator itr;
  for (itr = mDependents.begin(); itr != mDependents.end(); itr++) {
    SharedResourcePtr resourcePtr = itr->lock();
    if (resourcePtr) {
      std::tr1::shared_ptr<GraphicsResourceAsset> assetPtr =
          std::tr1::dynamic_pointer_cast<GraphicsResourceAsset>(*mDependencies.begin());
      if (assetPtr) {
          //resourcePtr->resolveName(mURI, assetPtr->getRemoteFileId());
          resourcePtr->resolveName(mURI);
      } else {
        SILOG(resource,fatal,"Fatal: Dependency of GraphicsResourceName is not a GraphicsResourceAsset" << getID() << " -> " << (*mDependencies.begin())->getID());
        resourcePtr->resolveName(mURI);//, ResourceHash(URI((*mDependencies.begin())->getID())));
      }
    }
  }
}

void GraphicsResourceName::addDependent(Meru::WeakResourcePtr newParent)
{
  if (mParseState == PARSE_VALID) {
    std::tr1::shared_ptr<GraphicsResourceAsset> assetPtr =
        std::tr1::dynamic_pointer_cast<GraphicsResourceAsset>(*mDependencies.begin());
    if (assetPtr) {
        newParent.lock()->resolveName(mURI); //, assetPtr->getRemoteFileId());
    } else {
      SILOG(resource,fatal,"Fatal: Dependency of GraphicsResourceName is not a GraphicsResourceAsset" << getID() << " -> " << (*mDependencies.begin())->getID());
      newParent.lock()->resolveName(mURI);//, ResourceHash(URI((*mDependencies.begin())->getID())));
    }
  }
  GraphicsResource::addDependent(newParent);
}

}

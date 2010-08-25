/*  Meru
 *  GraphicsResource.hpp
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
#ifndef _GRAPHICS_RESOURCE_HPP_
#define _GRAPHICS_RESOURCE_HPP_

#include "../meruCompat/MeruDefs.hpp"

#include <stdio.h>
#include <sirikata/proxyobject/ProxyMeshObject.hpp>
#include <sirikata/proxyobject/ProxyObject.hpp>

using namespace std;

namespace Meru {

class GraphicsResource;
typedef std::tr1::shared_ptr<GraphicsResource> SharedResourcePtr;
typedef std::tr1::weak_ptr<GraphicsResource> WeakResourcePtr;

class GraphicsResource : public SelfWeakPtr<GraphicsResource>
{
public:
  enum Type {
    NAME,
    ENTITY,
    MESH,
    MATERIAL,
    SKELETON,
    TEXTURE,
    SHADER,
    MODEL
  };

  enum ParseState {
    PARSE_INVALID,     // current parse not valid -- needs parse
    PARSE_PARSING,     // parsing newer dependencies
    PARSE_VALID,       // ready to load
    PARSE_FAILED       // failed to parse
  };

  enum LoadState {
    LOAD_NEW,
    LOAD_UNLOADED,
    LOAD_WAITING_LOAD,
    LOAD_LOADING,
    LOAD_LOADED,
    LOAD_WAITING_UNLOAD,
    LOAD_UNLOADING,
    LOAD_FAILED,
    LOAD_SWAPPABLE
  };

    GraphicsResource(const String &id, Type type, Sirikata::ProxyObjectPtr proxy);
  virtual ~GraphicsResource();

  inline const String &getID() const {
    return mID;
  }

  void parse();
  void load(unsigned int epoch);
  void unload(unsigned int epoch);

  void updateValue(unsigned int epoch);
  void updateCurCost(float cost, unsigned int epoch);
  float cost();
  void setCost(float cost);
  float getDepCost(unsigned int epoch);
  float value() const;

  virtual void parsed(bool success);
  virtual void loaded(bool success, unsigned int epoch);
  virtual void unloaded(bool success, unsigned int epoch);

  virtual void resolveName(const URI& idh) {
  }

  void removeLoadDependencies(unsigned int epoch);

  void addDependency(SharedResourcePtr newResource) {
    assert(newResource);
    mDependencies.insert(newResource);
    newResource->addDependent(getWeakPtr());
  }

  Type getType() {
    return mType;
  }

  ParseState getParseState();
  LoadState getLoadState();

    Sirikata::ProxyObjectPtr mProxy;

protected:
  static unsigned int sCostPropEpoch;

  virtual void doParse() = 0;
  virtual void doLoad() = 0;
  virtual void doUnload() = 0;
  virtual float calcBenefit() {
    return 0.0f;
  } // override if different

  virtual void addDependent(WeakResourcePtr newParent) {
    mDependents.insert(newParent);
  }

  void removeDependent(WeakResourcePtr parent) {
    mDependents.erase(parent);
  }

  void addDepBenefit(float benefit, unsigned int epoch);
  //void addDepCost(float cost, unsigned int epoch);

  void clearDependencies();
  void parseDependencies();
  void checkDependenciesParsed();
  void alertDependentsParsed(bool success);
  void dependencyParsed(bool success);
  virtual void fullyParsed();

  void loadDependencies();
  void checkDependenciesLoaded();
  void alertDependentsLoaded(bool success);
  void dependencyLoaded(bool success);

  void unloadDependents();
  void checkDependentsUnloaded();
  void alertDependenciesUnloaded(bool success);
  void dependentUnloaded(bool success);

  const String mID;
  ParseState mParseState;

  LoadState mLoadState;
  Type mType;

  std::set<SharedResourcePtr> mDependencies;
  std::set<WeakResourcePtr> mDependents;

  unsigned int mCostEpoch;
  unsigned int mLoadEpoch;
  unsigned int mRemoveEpoch;
  unsigned int mCostPropEpoch;
  float mBenefit;
  float mDepBenefit;
  float mCurCost;
  float mCost;
  float mDepCost;
};

}

#endif

/*  Meru
 *  GraphicsResource.cpp
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
#include "GraphicsResource.hpp"
#include "GraphicsResourceManager.hpp"

using namespace std;

namespace Meru {

unsigned int GraphicsResource::sCostPropEpoch = 0;

GraphicsResource::GraphicsResource(const String &id, Type type, Sirikata::ProxyObjectPtr proxy)
: mID(id), mParseState(PARSE_INVALID), mLoadState(LOAD_NEW),
  mType(type), mProxy(proxy), mCostEpoch(0), mLoadEpoch(0), mRemoveEpoch(0),
  mCostPropEpoch(0), mBenefit(0), mDepBenefit(0),
  mCurCost(0), mCost(0), mDepCost(0)
{

}

GraphicsResource::~GraphicsResource()
{
  GraphicsResourceManager::getSingleton().unregisterResource(this);
}

void GraphicsResource::updateValue(unsigned int epoch)
{
  assert(mType == ENTITY);

  float oldValue = value();
  mBenefit = calcBenefit();
  mCurCost = mCost + mDepCost;

  if (mLoadState != LOAD_NEW && mLoadState != LOAD_FAILED)
    GraphicsResourceManager::getSingleton().registerLoad(this, oldValue);

  if (mCost == 0.0f && mDependencies.size() == 1) { // we can trickle up benefit
    (*(mDependencies.begin()))->addDepBenefit(mBenefit, epoch);
  }
}

void GraphicsResource::addDepBenefit(float benefit, unsigned int epoch)
{
  float oldValue = value();

  if (mCostEpoch != epoch)
    mDepBenefit = 0.0f;
  mCostEpoch = epoch;
  mDepBenefit += benefit;
  mCurCost = mCost + mDepCost;

  if (mLoadState != LOAD_NEW && mLoadState != LOAD_FAILED
   && (mType == MESH || mType == ENTITY))
    GraphicsResourceManager::getSingleton().registerLoad(this, oldValue);

  if (mCost == 0.0f && mDependencies.size() == 1) { // we can trickle up benefit
    (*(mDependencies.begin()))->addDepBenefit(benefit, epoch);
  }
}

void GraphicsResource::setCost(float cost)
{
  assert(mCost == 0); // we should not be setting cost more than once
  mCost = cost;
}

float GraphicsResource::getDepCost(unsigned int epoch)
{
  float cost = 0;
  if (mCostPropEpoch != epoch) {
    mCostPropEpoch = epoch;

    cost += mCost;

    set<SharedResourcePtr>::iterator itr, eitr;
    for (itr = mDependencies.begin(), eitr = mDependencies.end(); itr != eitr; ++itr) {
        cost += (*itr)->getDepCost(epoch);
    }
  }
  return cost;
}

void GraphicsResource::updateCurCost(float cost, unsigned int epoch)
{
  if (mCostPropEpoch != epoch) {
    mCostPropEpoch = epoch;
    float oldValue = value();
    mCurCost -= cost;

    if (mType == MESH || mType == ENTITY)
      GraphicsResourceManager::getSingleton().updateLoadValue(this, oldValue);

    set<WeakResourcePtr>::iterator itr, eitr;
    for (itr = mDependents.begin(), eitr = mDependents.end(); itr != eitr; ++itr) {
      SharedResourcePtr resourcePtr = itr->lock();
      if (resourcePtr)
        resourcePtr->updateCurCost(cost, epoch);
    }
  }
}

float GraphicsResource::cost()
{
  return mCurCost;
}

float GraphicsResource::value() const
{
  float totalBenefit = mBenefit + mDepBenefit;
  if (totalBenefit == 0)
    return 0;
  if (mCurCost == 0)
    return std::numeric_limits<float>::max();

  return (totalBenefit) / mCurCost;
}

void GraphicsResource::parse()
{
  assert(mParseState == PARSE_INVALID);

  mParseState = PARSE_PARSING;
  doParse();
}

void GraphicsResource::parsed(bool success)
{
  if (success) {
    parseDependencies();
    checkDependenciesParsed();
  }
  else
    mParseState = PARSE_FAILED;
}

void GraphicsResource::parseDependencies()
{
  set<SharedResourcePtr>::iterator itr;

  for (itr = mDependencies.begin(); itr != mDependencies.end(); itr++) {
    ParseState state = (*itr)->getParseState();
    if (state == PARSE_INVALID) {
      (*itr)->parse();
    }
  }
}

void GraphicsResource::checkDependenciesParsed()
{
  bool parsed = true;
  set<SharedResourcePtr>::iterator itr;
  for (itr = mDependencies.begin(); itr != mDependencies.end(); itr++) {
    ParseState state = (*itr)->getParseState();
    if (state != PARSE_VALID) {
      parsed = false;
    }
  }

  if (parsed && mParseState == PARSE_PARSING) {
    mParseState = PARSE_VALID;
    if (mLoadState == LOAD_NEW)
      mLoadState = LOAD_UNLOADED;
    alertDependentsParsed(true);
    fullyParsed();
  }
}

void GraphicsResource::fullyParsed()
{
  sCostPropEpoch++;
  assert(mDepCost == 0);

  set<SharedResourcePtr>::iterator itr, eitr;
  for (itr = mDependencies.begin(), eitr = mDependencies.end(); itr != eitr; ++itr) {
    mDepCost += (*itr)->getDepCost(sCostPropEpoch);
  }
}

void GraphicsResource::dependencyParsed(bool success)
{
  if (success)
    checkDependenciesParsed();
}

void GraphicsResource::alertDependentsParsed(bool succes)
{
  std::set<WeakResourcePtr>::iterator itr;
  for (itr = mDependents.begin(); itr != mDependents.end(); itr++) {
    SharedResourcePtr resource = itr->lock();
    if (resource) {
      resource->dependencyParsed(succes);
    }
  }
}

void GraphicsResource::clearDependencies()
{
  std::set<SharedResourcePtr>::iterator itr;
  WeakResourcePtr weakPtr = getWeakPtr();
  for (itr = mDependencies.begin(); itr != mDependencies.end(); itr++) {
    (*itr)->removeDependent(weakPtr);
  }
  mDependencies.clear();
  mParseState = PARSE_INVALID;
}

GraphicsResource::ParseState GraphicsResource::getParseState() {
  return mParseState;
}

void GraphicsResource::load(unsigned int epoch)
{
  assert(mLoadEpoch != epoch);
  mLoadEpoch = epoch;
  mLoadState = LOAD_WAITING_LOAD;
  loadDependencies();
  checkDependenciesLoaded();
}

void GraphicsResource::removeLoadDependencies(unsigned int epoch)
{
  if (mRemoveEpoch != epoch) {
    mRemoveEpoch = epoch;

    if (mCost > 0) {
      sCostPropEpoch++;
      set<WeakResourcePtr>::iterator itr, eitr;
      for (itr = mDependents.begin(), eitr = mDependents.end(); itr != eitr; ++itr) {
        SharedResourcePtr resourcePtr = itr->lock();
        if (resourcePtr) {
          resourcePtr->updateCurCost(mCost, sCostPropEpoch);
        }
      }
    }

    GraphicsResourceManager::getSingleton().unregisterLoad(this);
    set<SharedResourcePtr>::iterator ditr;
    for (ditr = mDependencies.begin(); ditr != mDependencies.end(); ditr++) {
      (*ditr)->removeLoadDependencies(epoch);
    }
  }
}

void GraphicsResource::loadDependencies()
{
  set<SharedResourcePtr>::iterator itr;
  for (itr = mDependencies.begin(); itr != mDependencies.end(); itr++) {
    LoadState state = (*itr)->getLoadState();
    if (state != LOAD_WAITING_LOAD && state != LOAD_LOADED
     && state != LOAD_LOADING && state != LOAD_FAILED) {
      (*itr)->load(mLoadEpoch);
    }
  }
}

void GraphicsResource::checkDependenciesLoaded()
{
  if (mLoadState == LOAD_WAITING_LOAD) {
    bool loaded = true;
    set<SharedResourcePtr>::iterator itr;
    for (itr = mDependencies.begin(); itr != mDependencies.end(); itr++) {
      LoadState state = (*itr)->getLoadState();
      if (state != LOAD_LOADED)
        loaded = false;
    }

    if (loaded) {
      mLoadState = LOAD_LOADING;
      doLoad();
    }
  }
}

void GraphicsResource::dependencyLoaded(bool success)
{
  if (success)
    checkDependenciesLoaded();
}

void GraphicsResource::loaded(bool success, unsigned int epoch)
{
  if (mLoadEpoch == epoch) {
    if (success)
      mLoadState = LOAD_LOADED;
    else
      mLoadState = LOAD_FAILED;

    alertDependentsLoaded(success);
  }
}

void GraphicsResource::alertDependentsLoaded(bool succes)
{
  std::set<WeakResourcePtr>::iterator itr;
  for (itr = mDependents.begin(); itr != mDependents.end(); itr++) {
    SharedResourcePtr resource = itr->lock();
    if (resource) {
      resource->dependencyLoaded(succes);
    }
  }
}

void GraphicsResource::unload(unsigned int epoch)
{
  assert(mLoadState != LOAD_NEW);
  assert(mLoadEpoch != epoch);
  mLoadEpoch = epoch;
  mLoadState = LOAD_WAITING_UNLOAD;
  unloadDependents();
  checkDependentsUnloaded();
}

void GraphicsResource::unloadDependents()
{
  set<WeakResourcePtr>::iterator itr;
  for (itr = mDependents.begin(); itr != mDependents.end(); itr++) {
    SharedResourcePtr resourcePtr =  itr->lock();
    if (resourcePtr) {
      LoadState state = resourcePtr->getLoadState();
      if (state != LOAD_NEW && state != LOAD_UNLOADED
       && state != LOAD_UNLOADING && state != LOAD_WAITING_UNLOAD)
        resourcePtr->unload(mLoadEpoch);
    }
  }
}

void GraphicsResource::checkDependentsUnloaded()
{
  if (mLoadState == LOAD_WAITING_UNLOAD) {
    bool unloaded = true;
    set<WeakResourcePtr>::iterator itr;
    for (itr = mDependents.begin(); itr != mDependents.end(); itr++) {
      SharedResourcePtr resourcePtr =  itr->lock();
      if (resourcePtr) {
        LoadState state = resourcePtr->getLoadState();
        if (state != LOAD_UNLOADED)
          unloaded = false;
      }
    }

    if (unloaded) {
      mLoadState = LOAD_UNLOADING;
      doUnload();
    }
  }
}

void GraphicsResource::unloaded(bool success, unsigned int epoch)
{
  if (mLoadEpoch == epoch) {
    if (success)
      mLoadState = LOAD_UNLOADED;
    else
      assert(false); // we have a serious problem if we cannot unload something

    alertDependenciesUnloaded(success);
  }
}

void GraphicsResource::alertDependenciesUnloaded(bool success)
{
  std::set<SharedResourcePtr>::iterator itr;
  for (itr = mDependencies.begin(); itr != mDependencies.end(); itr++) {
    (*itr)->dependentUnloaded(success);
  }
}

void GraphicsResource::dependentUnloaded(bool success)
{
  if (success)
    checkDependentsUnloaded();
  else
    assert(false);
}

GraphicsResource::LoadState GraphicsResource::getLoadState() {
  return mLoadState;
}

}

/*  Sirikata -- Persistence Services
 *  ObjectStorage.cpp
 *
 *  Copyright (c) 2008, Stanford University
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


#include <util/Standard.hh>
#include "ObjectStorage.hpp"


namespace Sirikata {


StorageKey::StorageKey(const UUID& vwobj, uint32 obj, const String& field)
 : mVWObject(vwobj),
   mObject(obj),
   mField(field)
{
}

bool StorageKey::operator<(const StorageKey& rhs) const {
    return (mVWObject < rhs.mVWObject ||
        (mVWObject == rhs.mVWObject &&
            (mObject < rhs.mObject ||
                (mObject == rhs.mObject &&
                    (mField < rhs.mField)
                )
            )
        )
    );
}

String StorageKey::toString() const {
    std::stringstream ss;
    ss<<mVWObject.rawData()<<':'<<mObject<<':'<<mField;
    return ss.str();
}


bool StorageValue::operator==(const StorageValue& rhs) const {
    if (size() != rhs.size())
        return false;

    for(const_iterator lhs_it = begin(), rhs_it = rhs.begin(); lhs_it != end(); lhs_it++, rhs_it++) {
        if (*lhs_it != *rhs_it)
            return false;
    }

    return true;
}

bool StorageValue::operator!=(const StorageValue& rhs) const {
    return !(*this == rhs);
}


StorageSet::StorageSet() {
}

StorageSet::StorageSet(const StorageSet& cpy) {
    for(const_iterator it = cpy.begin(); it != cpy.end(); it++) {
        StorageValue* cpy_value = (it->second == NULL) ? NULL : new StorageValue(*(it->second));
        mMap[it->first] = cpy_value;
    }
}

StorageSet::~StorageSet() {
    for(iterator it = begin(); it != end(); it++)
        if (it->second != NULL)
            delete it->second;
}


StorageSet& StorageSet::operator=(const StorageSet& rhs) {
    for(const_iterator it = rhs.begin(); it != rhs.end(); it++) {
        StorageValue* cpy_value = (it->second == NULL) ? NULL : new StorageValue(*(it->second));
        mMap[it->first] = cpy_value;
    }
    return *this;
}

uint32 StorageSet::size() const {
    return mMap.size();
}

void StorageSet::addKey(const StorageKey& key) {
    assert(mMap.find(key) == mMap.end());
    mMap[key] = NULL;
}

void StorageSet::addPair(const StorageKey& key, const StorageValue& value) {
    assert(mMap.find(key) == mMap.end());
    mMap[key] = new StorageValue(value);
}

void StorageSet::addPair(const StorageKey& key, StorageValue* value) {
    assert(mMap.find(key) == mMap.end());
    mMap[key] = value;
}

void StorageSet::setValue(const StorageKey& key, const StorageValue& value) {
    iterator it = mMap.find(key);
    assert(it != mMap.end());

    if (it->second != NULL)
        delete it->second;

    it->second = new StorageValue(value);
}

void StorageSet::setValue(const StorageKey& key, StorageValue* value) {
    iterator it = mMap.find(key);
    assert(it != mMap.end());

    if (it->second != NULL)
        delete it->second;

    it->second = value;
}


const StorageValue& StorageSet::getValue(const StorageKey& key) const {
    const_iterator it = mMap.find(key);
    assert(it != mMap.end());

    return *(it->second);
}

bool StorageSet::hasKey(const StorageKey& key) const {
    return (mMap.find(key) != mMap.end());
}

StorageSet::iterator StorageSet::begin() {
    return mMap.begin();
}

StorageSet::const_iterator StorageSet::begin() const {
    return mMap.begin();
}

StorageSet::iterator StorageSet::end() {
    return mMap.end();
}

StorageSet::const_iterator StorageSet::end() const {
    return mMap.end();
}






ReadSet::ReadSet() {
}

ReadSet::ReadSet(const ReadSet& cpy)
 : StorageSet(cpy)
{
}

ReadSet::~ReadSet() {
}

ReadSet& ReadSet::operator=(const ReadSet& rhs) {
    StorageSet::operator=(rhs);
    return *this;
}




WriteSet::WriteSet() {
}

WriteSet::WriteSet(const WriteSet& cpy)
 : StorageSet(cpy)
{
}

WriteSet::~WriteSet() {
}

WriteSet& WriteSet::operator=(const WriteSet& rhs) {
    StorageSet::operator=(rhs);
    return *this;
}




CompareSet::CompareSet() {
}

CompareSet::CompareSet(const CompareSet& cpy)
 : StorageSet(cpy)
{
}

CompareSet::~CompareSet() {
}

CompareSet& CompareSet::operator=(const CompareSet& rhs) {
    StorageSet::operator=(rhs);
    return *this;
}



ReadWriteSet::ReadWriteSet()
 : mReads(),
   mWrites()
{
}

ReadWriteSet::ReadWriteSet(const ReadWriteSet& cpy)
 : mReads(cpy.reads()),
   mWrites(cpy.writes())
{
}

ReadWriteSet& ReadWriteSet::operator=(const ReadWriteSet& rhs) {
    mReads = rhs.reads();
    mWrites = rhs.writes();
    return *this;
}

ReadSet& ReadWriteSet::reads() {
    return mReads;
}

const ReadSet& ReadWriteSet::reads() const {
    return mReads;
}

WriteSet& ReadWriteSet::writes() {
    return mWrites;
}

const WriteSet& ReadWriteSet::writes() const {
    return mWrites;
}



Minitransaction::Minitransaction()
 : mCompares(),
   mReads(),
   mWrites()
{
}

Minitransaction::Minitransaction(const Minitransaction& cpy)
 : mCompares(cpy.compares()),
   mReads(cpy.reads()),
   mWrites(cpy.writes())
{
}

Minitransaction& Minitransaction::operator=(const Minitransaction& rhs) {
    mCompares = rhs.compares();
    mReads = rhs.reads();
    mWrites = rhs.writes();
    return *this;
}

CompareSet& Minitransaction::compares() {
    return mCompares;
}

const CompareSet& Minitransaction::compares() const {
    return mCompares;
}

ReadSet& Minitransaction::reads() {
    return mReads;
}

const ReadSet& Minitransaction::reads() const {
    return mReads;
}

WriteSet& Minitransaction::writes() {
    return mWrites;
}

const WriteSet& Minitransaction::writes() const {
    return mWrites;
}



ObjectStorageError::ObjectStorageError()
 : mType(ObjectStorageErrorType_None),
   mMsg()
{
}

ObjectStorageError::ObjectStorageError(const ObjectStorageErrorType t)
 : mType(t),
   mMsg()
{
}

ObjectStorageError::ObjectStorageError(const ObjectStorageErrorType t, const String& msg)
 : mType(t),
   mMsg(msg)
{
}

ObjectStorageErrorType ObjectStorageError::type() const {
    return mType;
}



ObjectStorageHandler::~ObjectStorageHandler() {
}

const UUID& ObjectStorageHandler::keyVWObject(const StorageKey& key) {
    return key.mVWObject;
}

uint32 ObjectStorageHandler::keyObject(const StorageKey& key) {
    return key.mObject;
}

const String& ObjectStorageHandler::keyField(const StorageKey& key) {
    return key.mField;
}



ReadWriteHandler::~ReadWriteHandler() {
}


MinitransactionHandler::~MinitransactionHandler() {
}




ObjectStorage::ObjectStorage(ReadWriteHandler* temp, ReadWriteHandler* bestEffort, MinitransactionHandler* trans)
 : mTemporary(temp),
   mBestEffort(bestEffort),
   mTransaction(trans)
{
}

void ObjectStorage::temporary(ReadWriteSet* rws, const ResultCallback& cb) {
    mTemporary->apply(rws,
        std::tr1::bind(&ObjectStorage::handleReadWriteResult, this, rws, cb, _1) );
}

void ObjectStorage::bestEffort(ReadWriteSet* rws, const ResultCallback& cb) {
    mBestEffort->apply(rws,
        std::tr1::bind(&ObjectStorage::handleReadWriteResult, this, rws, cb, _1) );
}

void ObjectStorage::transaction(Minitransaction* mt, const ResultCallback& cb) {
    mTransaction->apply(mt,
        std::tr1::bind(&ObjectStorage::handleTransactionResult, this, mt, cb, _1) );
}


void ObjectStorage::handleReadWriteResult(ReadWriteSet* rws, const ResultCallback& cb, const ObjectStorageError& error) {
    cb(error, rws->reads());
    delete rws;
}

void ObjectStorage::handleTransactionResult(Minitransaction* mt, const ResultCallback& cb, const ObjectStorageError& error) {
    cb(error, mt->reads());
    delete mt;
}


} // namespace Meru

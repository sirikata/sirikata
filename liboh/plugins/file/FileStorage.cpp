/*  Sirikata
 *  FileStorage.cpp
 *
 *  Copyright (c) 2011, Stanford University
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

#include "FileStorage.hpp"

namespace Sirikata {
namespace OH {

//Events

class FileStorageEvent
{
public:
    FileStorageEvent(const boost::filesystem::path& path)
     : mPath(path)
    {}
    virtual bool processEvent(Storage::ReadSet* rs) =0;

    virtual ~FileStorageEvent(){}
protected:
    const boost::filesystem::path mPath;
};

class FileStorageWriteItem : public FileStorageEvent
{
public:
    FileStorageWriteItem(const boost::filesystem::path& path, const String& whatToWrite)
     : FileStorageEvent(path),
       toWrite(whatToWrite)
    {}
    virtual bool processEvent(Storage::ReadSet* rs)
    {
        String fileToWrite = mPath.string();
        std::ofstream fWriter (fileToWrite.c_str(),  std::ios::out | std::ios::binary);

        for (String::size_type s = 0; s < toWrite.size(); ++s)
        {
            char toWriteChar = toWrite[s];
            fWriter.write(&toWriteChar,sizeof(toWriteChar));
        }

        fWriter.flush();
        fWriter.close();

        return true;
    }

    ~FileStorageWriteItem()
    {}
private:
    String toWrite;
};

class FileStorageClearItem : public FileStorageEvent
{
public:
    FileStorageClearItem(const boost::filesystem::path& path)
     : FileStorageEvent(path)
    {}
    virtual bool processEvent(Storage::ReadSet* rs)
    {
        if (! boost::filesystem::exists(mPath))
            return true;

        boost::filesystem::remove(mPath);
        return true;
    }

    ~FileStorageClearItem()
    {}
};

class FileStorageReadItem : public FileStorageEvent
{
public:
    FileStorageReadItem(const boost::filesystem::path& path, const Storage::Key& key)
     : FileStorageEvent(path),
       mKey(key)
    {}
    virtual bool processEvent(Storage::ReadSet* rs)
    {
        if (!boost::filesystem::exists(mPath))
            return false;

        String fileToRead = mPath.string();
        std::ifstream fRead(fileToRead.c_str(), std::ios::binary | std::ios::in);
        std::ifstream::pos_type begin, end;

        begin = fRead.tellg();
        fRead.seekg(0,std::ios::end);
        end   = fRead.tellg();
        fRead.seekg(0,std::ios::beg);

        std::ifstream::pos_type size = end-begin;
        String& lhs = (*rs)[mKey];
        lhs.resize(size);
        fRead.read(&lhs[0],size);

        return true;
    }

    ~FileStorageReadItem()
    {}
private:
    Storage::Key mKey;
};



FileStorage::FileStorage(ObjectHostContext* ctx, const String& dir)
 : mContext(ctx),
   mDir(dir)
{
    if (!boost::filesystem::exists(mDir))
        boost::filesystem::create_directory(mDir);
}


/**
   Frees memory associated with all outstanding events that have not been
   flushed.  DOES NOT FLUSH EVENTS.
 */
FileStorage::~FileStorage()
{
    std::vector<Bucket> allEntries;
    for (OutEventsIter iter = unflushedEvents.begin(); iter != unflushedEvents.end(); ++iter)
        allEntries.push_back(iter->first);

    for (std::vector<Bucket>::iterator strIter = allEntries.begin(); strIter != allEntries.end(); ++strIter)
        clearOutstanding(*strIter);
}

boost::filesystem::path FileStorage::getStoragePath(const Bucket& bucket) {
    return mDir / bucket.toString();
}

boost::filesystem::path FileStorage::getStoragePath(const Bucket& bucket, const Key& key) {
    return mDir / bucket.toString() / key;
}

void FileStorage::beginTransaction(const Bucket& bucket) {
    if (mActiveTransactions.find(bucket) == mActiveTransactions.end())
        mActiveTransactions.insert(bucket);
}

void FileStorage::commitTransaction(const Bucket& bucket, const CommitCallback& cb) {
    // Clear active transaction since we're finishing it
    if (mActiveTransactions.find(bucket) != mActiveTransactions.end())
        mActiveTransactions.erase(bucket);

    ReadSet* rs = NULL;

    if(!haveUnflushedEvents(bucket))
        if (cb) mContext->mainStrand->post(std::tr1::bind(cb, false, rs));

    rs = new ReadSet;
    std::vector<FileStorageEvent*> fbeVec = unflushedEvents[bucket];
    bool success = true;
    for (FileEventVecIter iter = fbeVec.begin(); iter != fbeVec.end(); ++iter)
        success = success && (*iter)->processEvent(rs);

    clearOutstanding(bucket);

    if (rs->empty() || !success) {
        delete rs;
        rs = NULL;
    }

    if (cb) mContext->mainStrand->post(std::tr1::bind(cb, success, rs));
}

bool FileStorage::haveUnflushedEvents(const Bucket& bucket)
{
    OutEventsIter iter = unflushedEvents.find(bucket);
    return (iter !=  unflushedEvents.end());
}


bool FileStorage::erase(const Bucket& bucket, const Key& key, const CommitCallback& cb)
{
    FileStorageClearItem* fbci = new FileStorageClearItem(getStoragePath(bucket, key));
    unflushedEvents[bucket].push_back(fbci);

    // Run commit if this is a one-off transaction
    if (mActiveTransactions.find(bucket) == mActiveTransactions.end())
        commitTransaction(bucket, cb);

    return true;
}


bool FileStorage::write(const Bucket& bucket, const Key& key, const String& strToWrite, const CommitCallback& cb)
{
    if (!boost::filesystem::exists(getStoragePath(bucket)))
        boost::filesystem::create_directory(getStoragePath(bucket));

    FileStorageWriteItem* fbw = new FileStorageWriteItem(getStoragePath(bucket, key), strToWrite);
    unflushedEvents[bucket].push_back(fbw);

    // Run commit if this is a one-off transaction
    if (mActiveTransactions.find(bucket) == mActiveTransactions.end())
        commitTransaction(bucket, cb);

    return true;
}


bool FileStorage::clearOutstanding(const Bucket& bucket)
{
    if(!haveUnflushedEvents(bucket))
        return false;

    OutEventsIter unflushedIter =  unflushedEvents.find(bucket);

    FileEventVec fev = unflushedIter->second;
    for (FileEventVecIter fevIter = fev.begin(); fevIter != fev.end(); ++fevIter)
    {
        FileStorageEvent* fbe = *fevIter;
        delete fbe;
        *fevIter = NULL;
    }

    unflushedEvents.erase(unflushedIter);

    return true;
}

bool FileStorage::read(const Bucket& bucket, const Key& key, const CommitCallback& cb)
{
    boost::filesystem::path path = getStoragePath(bucket, key);

    FileStorageReadItem* fbr = new FileStorageReadItem(getStoragePath(bucket, key), key);
    unflushedEvents[bucket].push_back(fbr);

    // Run commit if this is a one-off transaction
    if (mActiveTransactions.find(bucket) == mActiveTransactions.end())
        commitTransaction(bucket, cb);

    return true;
}


} //end namespace OH
} //end namespace Sirikata

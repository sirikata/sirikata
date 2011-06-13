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

//Write data contained in file
void FileStorageWriteItem::processEvent()
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
}

//Deletes item if it exists
void FileStorageClearItem::processEvent()
{
     if (! boost::filesystem::exists(mPath))
         return;

     boost::filesystem::remove(mPath);
 }





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

boost::filesystem::path FileStorage::getStoragePath(const Bucket& bucket, const String& prefix) {
    return mDir / bucket.toString() / prefix;
}

boost::filesystem::path FileStorage::getStoragePath(const Bucket& bucket, const String& prefix, const String& id) {
    return mDir / bucket.toString() / prefix / id;
}

void FileStorage::beginTransaction(const Bucket& bucket) {
}

void FileStorage::commitTransaction(const Bucket& bucket, const CommitCallback& cb) {
    if(! haveUnflushedEvents(bucket))
        if (cb) mContext->mainStrand->post(std::tr1::bind(cb, false));

    std::vector<FileStorageEvent*> fbeVec = unflushedEvents[bucket];
    for (FileEventVecIter iter = fbeVec.begin(); iter != fbeVec.end(); ++iter)
        (*iter)->processEvent();

    clearOutstanding(bucket);

    if (cb) mContext->mainStrand->post(std::tr1::bind(cb, true));
}

bool FileStorage::haveEntry(const Bucket& bucket, const String& prepend)
{
    return boost::filesystem::exists(getStoragePath(bucket, prepend));
}


bool FileStorage::haveUnflushedEvents(const Bucket& bucket)
{
    OutEventsIter iter = unflushedEvents.find(bucket);
    return (iter !=  unflushedEvents.end());
}


bool FileStorage::clearItem(const Bucket& bucket, const String& prependToken,const String& itemID)
{
    if(! haveUnflushedEvents(bucket))
        if (! haveEntry(bucket, prependToken))
            return false;

    FileStorageClearItem* fbci = new FileStorageClearItem(getStoragePath(bucket, prependToken,itemID));
    unflushedEvents[bucket].push_back(fbci);
    return true;
}


bool FileStorage::write(const Bucket& bucket, const String & prependToken, const String& idToWriteTo, const String& strToWrite)
{
    if(! haveUnflushedEvents(bucket)) {
        if (! haveEntry(bucket, prependToken)) {
            if (!boost::filesystem::exists(getStoragePath(bucket)))
                boost::filesystem::create_directory(getStoragePath(bucket));
            boost::filesystem::create_directory(getStoragePath(bucket, prependToken));
            assert(haveEntry(bucket, prependToken));
        }
    }

    FileStorageWriteItem* fbw = new FileStorageWriteItem(getStoragePath(bucket, prependToken,idToWriteTo),strToWrite);
    unflushedEvents[bucket].push_back(fbw);
    return true;
}


bool FileStorage::clearOutstanding(const Bucket& bucket)
{
    if(! haveUnflushedEvents(bucket))
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


bool FileStorage::clearEntry (const Bucket& bucket, const String& prependToken)
{
    //remove the folder from maps
    bool outstandingCleared = clearOutstanding(bucket);

    boost::filesystem::path path = getStoragePath(bucket, prependToken);
    if (! boost::filesystem::exists(path))
        return outstandingCleared || false;


    boost::filesystem::remove_all(path);
    return true;
}


bool FileStorage::read(const Bucket& bucket, const String& prepend, const String& idToReadFrom, String& toReadTo)
{
    boost::filesystem::path path = getStoragePath(bucket, prepend, idToReadFrom);

    if (! boost::filesystem::exists(path))
        return false;

    String fileToRead = path.string();

    std::ifstream fRead(fileToRead.c_str(), std::ios::binary | std::ios::in);
    std::ifstream::pos_type begin, end;

    begin = fRead.tellg();
    fRead.seekg(0,std::ios::end);
    end   = fRead.tellg();
    fRead.seekg(0,std::ios::beg);

    std::ifstream::pos_type size = end-begin;
    char* readBuf = new char[size];
    fRead.read(readBuf,size);

    String interString(readBuf,size);
    delete readBuf;
    toReadTo = interString;
    return true;
}


} //end namespace OH
} //end namespace Sirikata

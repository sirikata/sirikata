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
    std::vector<String> allEntries;
    for (OutEventsIter iter = unflushedEvents.begin(); iter != unflushedEvents.end(); ++iter)
        allEntries.push_back(iter->first);

    for (std::vector<String>::iterator strIter = allEntries.begin(); strIter != allEntries.end(); ++strIter)
        clearOutstanding(*strIter);
}

boost::filesystem::path FileStorage::getStoragePath(const String& prefix) {
    return mDir / prefix;
}

boost::filesystem::path FileStorage::getStoragePath(const String& prefix, const String& id) {
   return mDir / prefix / id;
}

bool FileStorage::haveEntry(const String& prepend)
{
    return boost::filesystem::exists(getStoragePath(prepend));
}


bool FileStorage::haveUnflushedEvents(const String& prepend)
{
    OutEventsIter iter = unflushedEvents.find(prepend);
    return (iter !=  unflushedEvents.end());
}


bool FileStorage::clearItem(const String& prependToken,const String& itemID)
{
    if(! haveUnflushedEvents(prependToken))
        if (! haveEntry(prependToken))
            return false;

    FileStorageClearItem* fbci = new FileStorageClearItem(getStoragePath(prependToken,itemID));
    unflushedEvents[prependToken].push_back(fbci);
    return true;
}


bool FileStorage::write(const String & prependToken, const String& idToWriteTo, const String& strToWrite)
{
    if(! haveUnflushedEvents(prependToken)) {
        if (! haveEntry(prependToken)) {
            boost::filesystem::create_directory(getStoragePath(prependToken));
            assert(haveEntry(prependToken));
        }
    }

    FileStorageWriteItem* fbw = new FileStorageWriteItem(getStoragePath(prependToken,idToWriteTo),strToWrite);
    unflushedEvents[prependToken].push_back(fbw);
    return true;
}


bool FileStorage::flush(const String& prependToken)
{
    if(! haveUnflushedEvents(prependToken))
        if (! haveEntry(prependToken))
            return false;

    std::vector<FileStorageEvent*> fbeVec = unflushedEvents[prependToken];
    for (FileEventVecIter iter = fbeVec.begin(); iter != fbeVec.end(); ++iter)
        (*iter)->processEvent();

    clearOutstanding(prependToken);

    return true;
}


bool FileStorage::clearOutstanding(const String& prependToken)
{
    if(! haveUnflushedEvents(prependToken))
        return false;

    OutEventsIter unflushedIter =  unflushedEvents.find(prependToken);

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


bool FileStorage::clearEntry (const String& prependToken)
{
    //remove the folder from maps
    bool outstandingCleared = clearOutstanding(prependToken);

    boost::filesystem::path path = getStoragePath(prependToken);
    if (! boost::filesystem::exists(path))
        return outstandingCleared || false;


    boost::filesystem::remove_all(path);
    return true;
}


bool FileStorage::read(const String& prepend, const String& idToReadFrom, String& toReadTo)
{
    boost::filesystem::path path = getStoragePath(prepend, idToReadFrom);

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

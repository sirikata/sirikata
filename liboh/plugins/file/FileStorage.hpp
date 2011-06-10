/*  Sirikata
 *  FileStorage.hpp
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

#ifndef __SIRIKATA_OH_STORAGE_FILE_HPP__
#define __SIRIKATA_OH_STORAGE_FILE_HPP__

#include <sirikata/oh/Storage.hpp>
#include <boost/filesystem.hpp>

namespace Sirikata {
namespace OH {

class FileStorageEvent
{
public:
    FileStorageEvent(const boost::filesystem::path& path)
     : mPath(path)
    {}
    virtual void processEvent() =0;

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
    virtual void processEvent();


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
    virtual void processEvent();

    ~FileStorageClearItem()
    {}
};



class FileStorage : public Storage
{
public:

    /**
       See documentation for Storage.  Key change: entries are
       folders, and items are the files in them.
     */
    FileStorage(ObjectHostContext* ctx, const String& dir);
    ~FileStorage();

    virtual void beginTransaction(const Bucket& bucket);
    virtual void commitTransaction(const Bucket& bucket, const CommitCallback& cb = 0);

    virtual bool haveEntry(const Bucket& bucket, const String& prepend);
    virtual bool clearItem(const Bucket& bucket, const String& prependToken,const String& itemID);
    virtual bool write(const Bucket& bucket, const String & prependToken, const String& idToWriteTo, const String& strToWrite);
    virtual bool clearEntry (const Bucket& bucket, const String& prepend);
    virtual bool read(const Bucket& bucket, const String& prepend, const String& idToReadFrom, String& toReadTo);

private:

    bool haveUnflushedEvents(const Bucket& bucket);
    bool clearOutstanding(const Bucket& bucket);

    boost::filesystem::path getStoragePath(const Bucket& bucket);
    boost::filesystem::path getStoragePath(const Bucket& bucket, const String& prefix);
    boost::filesystem::path getStoragePath(const Bucket& bucket, const String& prefix, const String& id);

    typedef std::vector<FileStorageEvent*> FileEventVec;
    typedef FileEventVec::iterator FileEventVecIter;
    typedef std::map<Bucket,std::vector<FileStorageEvent*> > OutstandingEvents;
    typedef OutstandingEvents::iterator OutEventsIter;

    ObjectHostContext* mContext;
    const boost::filesystem::path mDir; // Directory for storage
    //map of unflushed events indexed by entry (folder) name.
    OutstandingEvents unflushedEvents;
};

}//end namespace OH
}//end namespace Sirikata

#endif //__SIRIKATA_OH_STORAGE_FILE_HPP__

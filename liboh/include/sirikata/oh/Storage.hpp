/*  Sirikata
 *  Storage.hpp
 *
 *  Copyright (c) 2010, Stanford University
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

#ifndef __SIRIKATA_OH_STORAGE_HPP__
#define __SIRIKATA_OH_STORAGE_HPP__

#include <sirikata/core/util/Platform.hpp>
#include <sirikata/oh/Platform.hpp>
#include <sirikata/oh/HostedObject.hpp>
#include <sirikata/core/util/Factory.hpp>

namespace Sirikata {
namespace OH {

/** Represents a backing store for persistent object storage. Each object is
 *  assigned an identifier which maps to a 'bucket' of values in the backing
 *  store, keeping each object isolated. Within that bucket, objects can write
 *  to keys, specified as strings.
 */
class SIRIKATA_OH_EXPORT Storage {
public:

    virtual ~Storage() {};

    /**
       @param {String} entryName token to check if already have in our backend.
       @return {bool} returns true if already have an entry with this name in
       backend.  Otherwise, returns false.
    */
    virtual bool haveEntry(const String& entryName) = 0;

    /**
       @param{String} entryName.  Name of entry to check if already have a sequence of
       unflushed events for.

       @return {bool} Returns true if there are unflushed events still pending on
       this entry.
    */
    virtual bool haveUnflushedEvents(const String& entryName) = 0;


   /**
      @param{String} entryName, name of the entry in the backend.
      @param {String} itemName, name of the item in the entry.
      @return {bool} Returns true if have the entry in the backend.  Otherwise,
      returns false.

      Queues the item to be removed from the backend.  Does not actually delete
      until the flush operation is called.
   */
    virtual bool clearItem(const String& entryName,const String& itemName) = 0;


    /**
       Queues writes to the item named by entryName:itemName.  Writes
       will not be committed until flush command.  Note, if issue this command
       multiple times with the same entryName:itemName, will only process the
       last when calling flush.

       @param {String} entryName.  Used to identify the entry in the backend to
       write to.
       @param{String} itemName.  The name of the item to write to in the backend.
       @param{String} strToWrite.  What should actually be written into that item


       @return {bool} returns true if write is queued (ie if have the entry in the
       backend).  Otherwise, returns false
    */
    virtual bool write(const String & entryName, const String& itemName, const String& strToWrite) = 0;

    /**
       Flushes all outstanding events (writes and removes) from pending queue.
       Resets pending queue as well.


       @return {bool} returns true if the entryName matches a valid folder to write
       to.  Returns false otherwise.
    */
    virtual bool flush(const String& entryName) = 0;

    /**
       @param {string} entryName.  Name of entry to remove all pending events for.

       @return {bool} Returns true if have pending events for entryName.  Otherwise,
       returns false.

       Removes all pending events associated with entry with name entryName in
       the backend.
    */
    virtual bool clearOutstanding(const String& entryName) = 0;

    /**
       @param {String} entryName.  Name of entry to be removed from backend.

       @return {bool} returns true if have entry to clear.  Otherwise returns false.
    */
    virtual bool clearEntry (const String& entryName) = 0;


    /**
       @param {String} entryName.  Will read from item named entryName:itemName
       from backend.
       @param {String} itemName.  Will read from item named entryName:itemName
       from backend.
       @param {String} toReadTo.  Returns data from entryName:itemName by
       reference to this parameter.

       @return {bool} Returns true if an item and entry exist in the backend
       named entryName and itemName, respectively and the read operation on that
       item was successful.  Returns false otherwise.
     */
    virtual bool read(const String& entryName, const String& itemName, String& toReadTo) = 0;
};


class SIRIKATA_OH_EXPORT StorageFactory
    : public AutoSingleton<StorageFactory>,
      public Factory2<Storage*, ObjectHostContext*, const String&>
{
public:
    static StorageFactory& getSingleton();
    static void destroy();
};

} //end namespace OH
} //end namespace Sirikata

#endif //__SIRIKATA_OH_STORAGE_HPP__

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

/** Represents a backing store for persistent object storage. Each
 *  object is assigned an identifier which maps to a 'bucket' of
 *  values in the backing store, keeping each object isolated. Within
 *  that bucket, objects can write to keys, specified as strings.
 *
 *  Storage implementations *must* support transactions for atomic
 *  commits to multiple keys. There are two modes - transactions and
 *  single-key operations. In transactional mode, you call
 *  beginTransaction(), call other methods to add operations to the
 *  transaction, and finally call commit() to run the
 *  transaction. Single-key operations are called without any
 *  surrounding invokations of transaction methods. The implementation
 *  can send them to be performed immediately.  Single-key operations
 *  are really transactions, but elide the beginTransaction() and
 *  commit() calls.
 *
 *  Ordering of transactions (and single-key transactions) are not
 *  guaranteed: if you need to ensure operations happen in order you
 *  should either batch them in a transaction or wait for the callback
 *  for each one before processing the next.
 */
class SIRIKATA_OH_EXPORT Storage {
public:
    // A storage implementation contains a set of 'buckets' which
    // provide isolated storage for each object.
    typedef UUID Bucket;

    // Keys are arbitrary-length strings -- the namespace is flat, but
    // it is easy to add human readable hierarchy using a separator
    // (e.g. '.') and a bit of escaping.
    typedef String Key;

    /** CommitCallbacks are invoked */
    typedef std::tr1::function<void(bool success)> CommitCallback;

    virtual ~Storage() {};

    /** Begin a transaction. */
    virtual void beginTransaction(const Bucket& bucket) = 0;
    /** Completes a transaction and requests that it be written to
       Flushes all outstanding events (writes and removes) from pending queue.
       Resets pending queue as well.
    */
    virtual void commitTransaction(const Bucket& bucket, const CommitCallback& cb = 0) = 0;

   /**
      @param {Key} key the key to erase
      @return {bool} Returns true if have the entry in the backend.  Otherwise,
      returns false.

      Queues the item to be removed from the backend.  Does not actually delete
      until the flush operation is called.
   */
    virtual bool erase(const Bucket& bucket, const Key& key) = 0;


    /**
       Queues writes to the item named by entryName:itemName.  Writes
       will not be committed until flush command.  Note, if issue this command
       multiple times with the same entryName:itemName, will only process the
       last when calling flush.

       @param {Key} key the key to erase
       @param{String} value What should be written into that item

       @return {bool} returns true if write is queued (ie if have the entry in the
       backend).  Otherwise, returns false
    */
    virtual bool write(const Bucket& bucket, const Key& key, const String& value) = 0;

    /**
       @param {Key} key the key to erase
       @param {String} toReadTo.  Returns data from entryName:itemName by
       reference to this parameter.

       @return {bool} Returns true if an item and entry exist in the backend
       named entryName and itemName, respectively and the read operation on that
       item was successful.  Returns false otherwise.
     */
    virtual bool read(const Bucket& bucket, const Key& key, String& toReadTo) = 0;
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


#ifndef __JS_BACKEND_INTERFACE_HPP__
#define __JS_BACKEND_INTERFACE_HPP__

#include <sirikata/core/util/Platform.hpp>
#include <sirikata/oh/Platform.hpp>
#include <sirikata/oh/HostedObject.hpp>



namespace Sirikata{
namespace JS{


class JSBackendInterface
{
public:
    enum JSBackendCreateCode{
        BACKEND_CREATE_FAIL_HAVE_UNFLUSHED,
        BACKEND_CREATE_FAIL_EXISTS,
        BACKEND_CREATE_SUCCESS
    };
    
    virtual ~JSBackendInterface(){};


    /**
       @param {string} newEntryName.  The name of an entry to create in our
       backend.

       @return {JSBackendCreateCode} Returns
       JSBackendInterface::FAIL_HAVE_UNFINISHED if have pending events for this
       entry.  If don't have pending events for this entry, but the entry
       already exists on the system, returns JSBackendInterface::FAIL_EXISTS.
       Otherwise, returns JSBackendInterface::SUCCESS and creates an entry for
       this string token.
    */
    virtual JSBackendInterface::JSBackendCreateCode createEntry(const String & newEntryName) = 0;

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

}//end namespace JS
}//end namespace Sirikata

#endif



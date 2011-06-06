
#ifndef __JS_FILE_BACKEND_INTERFACE_HPP__
#define __JS_FILE_BACKEND_INTERFACE_HPP__

#include <map>
#include "JSBackendInterface.hpp"
#include <boost/filesystem.hpp>
#include <sirikata/core/util/Platform.hpp>

namespace Sirikata{
namespace JS{

class FileBackendEvent
{
public:
    FileBackendEvent(const String& prep, const String& id)
     : folderName(prep),
       fileName(id)
    {}
    virtual void processEvent() =0;
    
    virtual ~FileBackendEvent(){}
protected:
    String folderName,fileName;
};

class FileBackendWriteItem : public FileBackendEvent
{
public:
    FileBackendWriteItem(const String& prep, const String& id, const String& whatToWrite)
     : FileBackendEvent(prep,id),
       toWrite(whatToWrite)
    {}
    virtual void processEvent();

    
    ~FileBackendWriteItem()
    {}
private:
    String toWrite;
};

class FileBackendClearItem : public FileBackendEvent
{
public:
    FileBackendClearItem(const String& prep, const String& id)
         : FileBackendEvent(prep,id)
    {}
    virtual void processEvent();
    
    ~FileBackendClearItem()
    {}
};



class JSFileBackend : public JSBackendInterface
{
public:

    /**
       See documentation for JSBackendInterface.  Key change: entries are
       folders, and items are the files in them.
     */
    JSFileBackend();
    ~JSFileBackend();

    virtual JSBackendInterface::JSBackendCreateCode createEntry(const String & prepend);
    virtual bool haveEntry(const String& prepend);
    virtual bool haveUnflushedEvents(const String& prepend);
    virtual bool clearItem(const String& prependToken,const String& itemID);
    virtual bool write(const String & prependToken, const String& idToWriteTo, const String& strToWrite);
    virtual bool flush(const String& prependToken);
    virtual bool clearOutstanding(const String& prependToken);
    virtual bool clearEntry (const String& prepend);
    virtual bool read(const String& prepend, const String& idToReadFrom, String& toReadTo);
private:
    

    typedef std::vector<FileBackendEvent*> FileEventVec;
    typedef FileEventVec::iterator FileEventVecIter;
    typedef std::map<String,std::vector<FileBackendEvent*> > OutstandingEvents;
    typedef OutstandingEvents::iterator OutEventsIter;

    //map of unflushed events indexed by entry (folder) name.
    OutstandingEvents unflushedEvents;
};

}//end namespace JS
}//end namespace Sirikata

#endif 



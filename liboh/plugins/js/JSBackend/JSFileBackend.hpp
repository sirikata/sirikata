
#ifndef __JS_FILE_BACKEND_INTERFACE_HPP__
#define __JS_FILE_BACKEND_INTERFACE_HPP__

#include <map>
#include "JSBackendInterface.hpp"
#include <boost/filesystem.hpp>
#include <sirikata/core/util/Platform.hpp>

namespace Sirikata{
namespace JS{

class JSFileBackend : public JSBackendInterface
{
public:

    JSFileBackend();
    virtual ~JSFileBackend();

    virtual UUID createEntry(const String& prepend);
    virtual bool write(const UUID& seqKey, const String& idToWriteTo, const String& strToWrite);
    virtual bool flush(const UUID& seqKey);
    virtual bool read(const String& prepend, const String& idToReadFrom, String& toReadTo);
    
private:
    std::map<UUID,String> idsToFolderNames;
    std::map<UUID, std::map<String,String> > outstandingWrites;
    
};

}//end namespace JS
}//end namespace Sirikata

#endif 



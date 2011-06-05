
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
    virtual ~JSBackendInterface(){};

    virtual UUID createEntry(const String& prepend)=0;
    virtual bool clearEntry (const String& prepend)=0;
    
    virtual bool write(const UUID& seqKey, const String& idToWriteTo, const String& strToWrite)=0;
    virtual bool flush(const UUID& seqKey)=0;

    virtual bool read(const String& prepend, const String& idToReadFrom, String& toReadTo) = 0;
    
};

}//end namespace JS
}//end namespace Sirikata

#endif



#ifndef __SIRIKATA_JS_QUOTED_STRUCT_HPP__
#define __SIRIKATA_JS_QUOTED_STRUCT_HPP__

#include <sirikata/oh/HostedObject.hpp>
#include <v8.h>



namespace Sirikata {
namespace JS {


struct JSQuotedStruct
{
    JSQuotedStruct(const String& toQuote);
    ~JSQuotedStruct();

    static JSQuotedStruct* decodeQuotedStruct(v8::Handle<v8::Value> toDecode ,std::string& errorMessage);
    String getQuote();
    
private:
    String mString;
};


}//end namespace js
}//end namespace sirikata

#endif

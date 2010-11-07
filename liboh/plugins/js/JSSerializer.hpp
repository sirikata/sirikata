#ifndef __SIRIKATA_JS_SERIALIZE_HPP__
#define __SIRIKATA_JS_SERIALIZE_HPP__

#include <string>
#include "JS_JSMessage.pbj.hpp"

//#include <sirikata/core/util/RoutableMessageHeader.hpp>
//#include <sirikata/core/util/RoutableMessageBody.hpp>
//#include <sirikata/core/util/RoutableMessageBody.hpp>
#include <v8.h>

namespace Sirikata {
namespace JS {

/*
  FIXME: Need to Re-name these files to JSSerializer
 */

class JSSerializer
{
private:
	static std::string serializeFunction(v8::Local<v8::Function> v8Func);
public:
    static std::string serializeObject(v8::Local<v8::Value> v8Val);;
    static bool deserializeObject( std::string strDecode,v8::Local<v8::Object>& deserializeTo);
    //static bool deserializeObject( MemoryReference payload,v8::Local<v8::Object>& deserializeTo);
    static bool deserializeObject( Sirikata::JS::Protocol::JSMessage jsmessage,v8::Local<v8::Object>& deserializeTo);
};

}}//end namespaces

#endif

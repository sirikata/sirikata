
#ifndef __SIRIKATA_JS_GLOBAL_HPP__
#define __SIRIKATA_JS_GLOBAL_HPP__

#include <sirikata/oh/Platform.hpp>

#include "../JSObjectScriptManager.hpp"
#include "../JSObjectScript.hpp"

#include "../JSSerializer.hpp"



namespace Sirikata {
namespace JS {
namespace JSGlobal {

v8::Handle<v8::Value>checkResources(const v8::Arguments& args);

}//end jsglobal namespace
}//end js namespace
}//end sirikata

#endif

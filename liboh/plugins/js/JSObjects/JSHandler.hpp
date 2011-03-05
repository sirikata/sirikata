

#include "../JSUtil.hpp"
#include "../JSObjectScript.hpp"
#include <v8.h>


namespace Sirikata {
namespace JS {
namespace JSHandler{


v8::Handle<v8::Value> _printContents(const v8::Arguments& args);
v8::Handle<v8::Value> _suspend(const v8::Arguments& args);
v8::Handle<v8::Value> _resume(const v8::Arguments& args);
v8::Handle<v8::Value> _isSuspended(const v8::Arguments& args);
v8::Handle<v8::Value> _clear(const v8::Arguments& args);


void readHandler(const v8::Arguments& args, JSObjectScript*& caller, JSEventHandlerStruct*& hand);
v8::Handle<v8::Value> makeEventHandler(JSObjectScript* target_script, JSEventHandlerStruct* evHand);
void setNullHandler(const v8::Arguments& args);


}  //end jshandler namespace
}  //end js namespace
}  //end sirikata namespace

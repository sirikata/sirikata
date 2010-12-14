#ifndef __JS_FUNCTION_INVOKABLE_HPP__
#define __JS_FUNCTION_INVOKABLE_HPP__

#include <sirikata/oh/Platform.hpp>

#include <vector>
#include <v8.h>
#include <sirikata/proxyobject/Invokable.hpp>


namespace Sirikata
{

namespace JS
{
  class JSObjectScript;
  class JSFunctionInvokable : public Invokable
  {
    public:
    JSFunctionInvokable(v8::Persistent<v8::Function> _function, JSObjectScript* _script)
    : function_(_function), script_(_script){}
    
    boost::any invoke(std::vector<boost::any>& params);
    private:
    v8::Persistent<v8::Function> function_;
    JSObjectScript* script_;
  };

}


}

#endif

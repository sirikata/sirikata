#ifndef __JS_SIMULATION_OBJECT__
#define __JS_SIMULATION_OBJECT__

#include <sirikata/oh/Platform.hpp>

#include <vector>
#include <v8.h>
#include <sirikata/proxyobject/Invokable.hpp>



namespace Sirikata
{
namespace JS
{
  class JSObjectScript;

namespace JSInvokableObject
{

class JSInvokableObjectInt : public Invokable
{

  public:
  JSInvokableObjectInt(Invokable* _invokable) : invokable_(_invokable){}

  Invokable* invokable() const {return invokable_;}
//  JSObjectScript* script() const {return script_;}
  void invokableIs(Invokable* _invokable) {invokable_=_invokable;}
//  void scriptIs(JSObjectScript* _script) {script_=_script;}
  
  boost::any invoke(std::vector<boost::any>& params);

  
  private:

  Invokable* invokable_;
//  JSObjectScript* script_;

};

v8::Handle<v8::Value> invoke(const v8::Arguments& args);
bool decodeJSInvokableObject(v8::Handle<v8::Value>, JSObjectScript*&, JSInvokableObject::JSInvokableObjectInt*&);


}

}

}



#endif

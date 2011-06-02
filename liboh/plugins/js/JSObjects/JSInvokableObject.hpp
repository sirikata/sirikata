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
  class EmersonScript;

namespace JSInvokableObject
{

class JSInvokableObjectInt : public Invokable
{

  public:
  JSInvokableObjectInt(Invokable* _invokable) : invokable_(_invokable){}

  Invokable* invokable() const {return invokable_;}
  void invokableIs(Invokable* _invokable) {invokable_=_invokable;}
  
  boost::any invoke(std::vector<boost::any>& params);

  
  private:

  Invokable* invokable_;


};

v8::Handle<v8::Value> invoke(const v8::Arguments& args);
bool decodeJSInvokableObject(v8::Handle<v8::Value>, EmersonScript*&, JSInvokableObject::JSInvokableObjectInt*&);


}

}

}



#endif

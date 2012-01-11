#include <vector>
#include <boost/any.hpp>
#include <v8.h>


#include "JSFunctionInvokable.hpp"
#include "../EmersonScript.hpp"
#include "JSInvokableUtil.hpp"
#include "JSObjectsUtils.hpp"

namespace Sirikata
{
namespace JS
{

  boost::any JSFunctionInvokable::invoke(std::vector<boost::any>& params)
  {
      /* Invoke the function handle */
      return script_->invokeInvokable(params,function_);
  }
      

}
}

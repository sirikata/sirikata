#include <sirikata/proxyobject/Invokable.hpp>

namespace Sirikata
{
  boost::any Invokable::invoke(std::vector<boost::any>& params) {
    return boost::any();
  }
  Invokable::~Invokable() {
  }
}

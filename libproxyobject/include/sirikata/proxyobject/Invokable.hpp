#ifndef _SIRIKATA_INVOKABLE_HPP_
#define _SIRIKATA_INVOKABLE_HPP_

#include <sirikata/proxyobject/Platform.hpp>
#include <vector>
#include <boost/any.hpp>


namespace Sirikata
{
  class SIRIKATA_PROXYOBJECT_EXPORT Invokable
  {
    public:
    virtual boost::any invoke(std::vector<boost::any>& params);
    virtual ~Invokable();
  };
}

#endif

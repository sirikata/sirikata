#ifndef _CBR_OSEG_HASHER_HPP
#define _CBR_OSEG_HASHER_HPP

//#include <sirikata/util/Standard.hh>
#include <sirikata/util/Platform.hpp>
#include <sirikata/util/UUID.hpp>
#include <sirikata/util/boost_uuid.hpp>
#include "Server.hpp"


namespace CBR
{

  class OSegHasher
  {
  public:
    OSegHasher();
    ~OSegHasher();
    static size_t hash(ServerID sID);
    //    uint32 hash(ServerID sID);
  };
}
  
#endif

#ifndef _SIRIKATA_OSEG_HASHER_HPP
#define _SIRIKATA_OSEG_HASHER_HPP

//#include <sirikata/core/util/Standard.hh>
#include <sirikata/core/util/Platform.hpp>
#include <sirikata/core/util/UUID.hpp>
#include "Server.hpp"


namespace Sirikata
{

  class OSegHasher
  {
  private:
    static const uint32 MAX_HASH_NUMBER = 5135;
  public:
    OSegHasher();
    ~OSegHasher();
    static uint32 hash(ServerID sID);
    static uint32 hash(const UUID& obj_id);


  };
}

#endif

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
  private:
    static const uint32 MAX_HASH_NUMBER = 80;
  public:
    OSegHasher();
    ~OSegHasher();
    static uint32 hash(ServerID sID);
    static uint32 hash(const UUID& obj_id);

    
  };
}
  
#endif

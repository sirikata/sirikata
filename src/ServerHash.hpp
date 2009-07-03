#ifndef _CBR_TEST_SHA1_HPP
#define _CBR_TEST_SHA1_HPP

//#include <sirikata/util/Standard.hh>
#include <sirikata/util/Platform.hpp>
#include <sirikata/util/UUID.hpp>
#include <sirikata/util/boost_uuid.hpp>
#include "Server.hpp"


namespace CBR
{

  class ServerHash
  {
  public:
    ServerHash();
    ~ServerHash();
    size_t hash(ServerID sID);
  };
}
  
#endif

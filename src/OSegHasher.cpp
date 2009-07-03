#include "OSegHasher.hpp"



namespace CBR
{

  OSegHasher::OSegHasher()
  {

 }


  OSegHasher::~OSegHasher()
  {

  }







  
  uint32 OSegHasher::hash(const UUID& obj_id)
  {
    return (uint32) (obj_id.hash() % MAX_HASH_NUMBER);
  }
  

  
  /*
    Largely derived from hashing found in Sirikata's uuid code.
    Not sure this works very well though.  Will likely have to modify it later.
  */
  uint32 OSegHasher::hash(ServerID sID)
  {
    uint64 a;
    uint64 b;

    
    a = sID;
    b = sID +8;
    static const uint32 MAX_HASH_NUMBER = 80;
    return (uint32)( (std::tr1::hash<uint64>()(a)^std::tr1::hash<uint64>()(b)) % MAX_HASH_NUMBER) ;
  }
}


#include "OSegHasher.hpp"



namespace Sirikata
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
    uint32 a;
    uint32 b;

    uint32 tmp1,tmp2,tmp3,tmp4;

    /*
    tmp1 = (sID>>16);
    tmp2 = (sID>>16)<<16;
    tmp3 = (sID<<16);
    tmp4 = (sID<<16)>>16;
    
    std::cout<<"\n\n\t"<<sID<<"   "<<tmp1<<"  "<<tmp2<<"\n";
    std::cout<<"\n\t"<<sID<<"   "<<tmp3<<"  "<<tmp4<<"\n\n\n";
    */
    a = sID;
    b = (sID<<16);

    return (uint32)( (std::tr1::hash<uint32>()(a)^std::tr1::hash<uint32>()(b)) % MAX_HASH_NUMBER) ;
  }
}


#include "ServerHash.hpp"



namespace CBR
{

  ServerHash::ServerHash()
  {

 }


  ServerHash::~ServerHash()
  {

  }

  
  
  //server id is uint32
  size_t ServerHash::hash(ServerID sID)
 {
    uint64 a;
    uint64 b;

    a = sID;
    b = sID * 32;
    
    //    memcpy(&a,mData.begin(),sizeof(a));
   //    memcpy(&b,mData.begin()+8,sizeof(b));
    return std::tr1::hash<uint64>()(a)^std::tr1::hash<uint64>()(b);
  }
}


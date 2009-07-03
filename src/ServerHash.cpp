#include "OSegHasher.hpp"



namespace CBR
{

  OSegHasher::OSegHasher()
  {

 }


  OSegHasher::~OSegHasher()
  {

  }

  
  
  //server id is uint32
  size_t OSegHasher::hash(ServerID sID)
  //  uint32 ServerHash::hash(ServerID sID)
  {
        uint64 a;
        uint64 b;

    //uint32 a;
    //    uint32 b;
    
    a = sID;
    b = sID +8;
    
    //    memcpy(&a,mData.begin(),sizeof(a));
   //    memcpy(&b,mData.begin()+8,sizeof(b));
    return std::tr1::hash<uint64>()(a)^std::tr1::hash<uint64>()(b);
    //    return std::tr1::hash<uint32>()(a)^std::tr1::hash<uint32>()(b);
  }
}


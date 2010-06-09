/*  Sirikata
 *  OSegHasher.cpp
 *
 *  Copyright (c) 2010, Daniel Reiter Horn
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Sirikata nor the names of its contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

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

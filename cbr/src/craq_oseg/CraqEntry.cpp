/*  Sirikata
 *  CraqEntry.cpp
 *
 *  Copyright (c) 2010, Behram Mistree
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

#include <arpa/inet.h>
#include <sirikata/cbrcore/Utility.hpp>
#include "asyncUtil.hpp"
#include "CraqEntry.hpp"
#include "Base64.hpp"

namespace Sirikata {
void CraqEntry::deserialize(unsigned char xinput[CRAQ_SERVER_SIZE]) {
    unsigned char input[12]={0};
    input[11]='=';
    input[10]='=';
    memcpy(input,xinput,CRAQ_SERVER_SIZE);
    unsigned char base256osegData[10]={0};
    decode12Base64(base256osegData+1,input);
    uint32_t radiusbe=0;
    uint32_t serverbe=0;
    memcpy(&serverbe,base256osegData,4);
    memcpy(&radiusbe,base256osegData+4,4);
    uint32_t radiusle=ntohl(radiusbe);
    mServer=ntohl(serverbe);
    char radiusArray[sizeof(float)];
    memcpy(radiusArray,&radiusle,sizeof(float));
    memcpy(&mRadius,radiusArray,sizeof(float));
}
void CraqEntry::serialize(unsigned char output[CRAQ_SERVER_SIZE]) const{
  char radiusle[sizeof(float)];
  memcpy(radiusle,&mRadius,sizeof(float));
  uint32_t radiusbe;
  memcpy(&radiusbe,radiusle,sizeof(uint32_t));
  radiusbe=htonl(radiusbe);
  unsigned int inputDatabe=htonl(mServer);
  unsigned char base256osegData[10]={0};
  memcpy(base256osegData,&inputDatabe,4);
  memcpy(base256osegData+4,&radiusbe,4);

  unsigned char xoutput[12];
  encode9Base64(base256osegData+1,xoutput);
  memcpy(output,xoutput,CRAQ_SERVER_SIZE);
}
static bool craqSerializationUnitTest(){
    CraqEntry input(123589,15985.129);
    unsigned char output[CRAQ_SERVER_SIZE+1];
    output [CRAQ_SERVER_SIZE]='\0';
    input.serialize(output);
    CraqEntry result(output);
    bool failed=false;
    if (input.server()!=result.server()) {
        fprintf (stderr,"%d != %d serialized:%s\n",input.server(),result.server(),output);
    }
    if (input.radius()!=result.radius()) {
        fprintf (stderr,"%f != %f  serialized:%s\n",input.radius(),result.radius(),output);
    }

    assert(input.server()==result.server());
    assert(input.radius()==result.radius());
    output[0]=';';
    output[1]=':';
    output[2]='_';
    output[3]='-';
    output[4]='0';
    CraqEntry hacked(output);
    unsigned char hackedoutput[CRAQ_SERVER_SIZE];
    hacked.serialize(hackedoutput);
    assert(memcmp(hackedoutput,output,CRAQ_SERVER_SIZE)==0);
    if (memcmp(hackedoutput,output,CRAQ_SERVER_SIZE)!=0){
        printf("CRAQ SERIALIZATION failure with : and ;\n");
    }
    if (input.server()!=result.server()||input.radius()!=result.radius()) {
        while(true) {
            static bool test=true;
            if (test) {
                printf("failure\n");
            }
            test=false;
        }
    }
    return true;
}
bool didthiswork=craqSerializationUnitTest();
}

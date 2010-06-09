/*  Sirikata
 *  asyncCraqSetPoolTest.cpp
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

#include <iostream>
#include <iomanip>

#include "asyncCraqSet.hpp"
#include <boost/asio.hpp>
#include <string.h>
#include <vector>
#include "asyncConnectionSet.hpp"
#include "../asyncCraqUtil.hpp"

#include <sys/time.h>

#define WHICH_SERVER  "localhost"
#define WHICH_PORT        "10499"
//#define WHICH_SERVER  "bmistree.stanford.edu"
//#define WHICH_PORT        "4999"


namespace Sirikata
{
  void basicWait(AsyncCraqSet* myReader,std::vector<CraqOperationResult*> &allGetResults,std::vector<CraqOperationResult*>&allTrackedResults);
  void iteratedWait(int numWaits,AsyncCraqSet* myReader,std::vector<CraqOperationResult*> &allGetResults,std::vector<CraqOperationResult*>&allTrackedResults);
  void printResponse(CraqOperationResult* res);

  //static const int numPuts = 400;
  static const int numPuts = 1000;
  static const int numInit = 10;

  void runLoad();
  void runUnLoad();
  void runMultiUnLoad(int numTimesLoad);

}

int main (int argc, char** argv)
{
  CBR::runLoad();

  //  runMultiUnLoad(1);
  //  runUnLoad();

  return 0;
}

namespace Sirikata
{

  void runLoad()
  {
    AsyncCraqSet myReader1(NULL,NULL);


    //Declare craq args
    std::vector<CraqInitializeArgs> craqArgs1;
    CraqInitializeArgs cInitArgs;
    //    cInitArgs.ipAdd   =     "bmistree.stanford.edu";
    cInitArgs.ipAdd   =    WHICH_SERVER;
    cInitArgs.port    =      WHICH_PORT;


    craqArgs1.push_back(cInitArgs);

    std::vector<CraqOperationResult*> allGets;
    std::vector<CraqOperationResult*> allTrackedSets;


    std::cout<<"\n\nAbout to perform reader initialization.\n\n";
    std::cout.flush();
    myReader1.initialize(craqArgs1);


    for(int s=0; s < 10000; ++s)
    {
      iteratedWait(1,&myReader1,allGets, allTrackedSets);
      //    iteratedWait(1,&myReader2,allGets, allTrackedSets);
    }


    std::cout<<"\n\nAbout to perform first set of sets\n\n";
    std::cout.flush();
    //performing first sets
    CraqDataKey cDK;
    std::string query;


    struct timeval first_time, finished_time;
    gettimeofday(&first_time,NULL);



    for (int s=numInit; s < numInit+ numPuts; ++s)
    {
      query = "";
      std::stringstream ss;
      ss << s;
      std::string tmper = ss.str();

      for (int t=0; t< CRAQ_DATA_KEY_SIZE - (((int)tmper.size()) + 1); ++t)
      {
        query.append("0");
      }
      query.append(tmper);


      CraqDataSetGet cdSetGet (query,s,false,CraqDataSetGet::SET);
      myReader1.set(cdSetGet);

      iteratedWait(1,&myReader1,allGets,allTrackedSets);

    }

    std::cout<<"\n\nAbout to go through queue iterations\n\n";

    while(myReader1.queueSize() != 0)
    {
      iteratedWait(10000,&myReader1,allGets, allTrackedSets);
      std::cout<<"\n\nThis is queue size:  "<<myReader1.queueSize()<<"\n\n";
      std::cout.flush();
    }

    iteratedWait(70000,&myReader1,allGets, allTrackedSets);


    int allGetsSize = (int) allGets.size();
    std::cout<<"\n\nThis is size of allGets:   "<<allGets.size()<<"\n\n";
    std::cout.flush();


    for (int s=0; s < (int)allGets.size(); ++ s)
    {
      printResponse(allGets[s]);
    }


    gettimeofday(&finished_time,NULL);
    double secondsElapsed  = finished_time.tv_sec - first_time.tv_sec;
    double uSecondsElapsed = finished_time.tv_usec - first_time.tv_usec;
    double realTimeTakenSeconds = secondsElapsed + uSecondsElapsed/(1000000);
    std::cout<<"\n\nThis is timer final:  "<<realTimeTakenSeconds<<"\n\n";
    std::cout<<"\n\nThis is size of allGets:   "<<allGetsSize<<"\n\n";
    std::cout.flush();
  }





  void runUnLoad()
  {
    //declare reader
    AsyncCraqSet myReader1(NULL,NULL);


    //declare craq args
    std::vector<CraqInitializeArgs> craqArgs1;
    CraqInitializeArgs cInitArgs;
    cInitArgs.ipAdd   =     WHICH_SERVER;
    cInitArgs.port    =       WHICH_PORT;




    craqArgs1.push_back(cInitArgs);

    std::vector<CraqOperationResult*> allGets;
    std::vector<CraqOperationResult*> allTrackedSets;


    std::cout<<"\n\nAbout to perform reader initialization.\n\n";
    myReader1.initialize(craqArgs1);


    for(int s=0; s < 10000; ++s)
    {
      iteratedWait(1,&myReader1,allGets, allTrackedSets);
      //    iteratedWait(1,&myReader2,allGets, allTrackedSets);
    }


    std::cout<<"\n\nAbout to perform first set of sets\n\n";
    //performing first sets
    CraqDataKey cDK;
    std::string query;



    struct timeval first_time, finished_time;
    gettimeofday(&first_time,NULL);



    for(int s=0; s < 100000; ++s)
    {
      iteratedWait(1,&myReader1,allGets, allTrackedSets);
      iteratedWait(1,&myReader1,allGets, allTrackedSets);
    }

    //about to perform gets
    std::cout<<"\n\nAbout to perform a bunch of gets\n\n";

    //  for (int s=0; s < numPuts; ++s)
    for (int s=numInit; s < numInit+ numPuts; ++s)
    {
      query = "";
      std::stringstream ss;
      ss << s;
      std::string tmper = ss.str();

      for (int t=0; t< CRAQ_DATA_KEY_SIZE - (((int)tmper.size()) + 1); ++t)
      {
        query.append("0");
      }
      query.append(tmper);


      CraqDataSetGet cdSetGet2 (query,s,false,CraqDataSetGet::GET);
      myReader1.get(cdSetGet2);

      iteratedWait(1,&myReader1,allGets,allTrackedSets);

    }

    //  iteratedWait(100000,&myReader,allGets, allTrackedSets);

    for(int s=0; s < 100000; ++s)
    {
      iteratedWait(1,&myReader1,allGets, allTrackedSets);
      iteratedWait(1,&myReader1,allGets, allTrackedSets);
    }

    for(int s=0; s < 100000; ++s)
    {
      iteratedWait(5,&myReader1,allGets, allTrackedSets);
      iteratedWait(5,&myReader1,allGets, allTrackedSets);
    }


    int allGetsSize = (int) allGets.size();
    std::cout<<"\n\nThis is size of allGets:   "<<allGets.size()<<"\n\n";


    for (int s=0; s < (int)allGets.size(); ++ s)
    {
      printResponse(allGets[s]);
    }

    gettimeofday(&finished_time,NULL);
    double secondsElapsed  = finished_time.tv_sec - first_time.tv_sec;
    double uSecondsElapsed = finished_time.tv_usec - first_time.tv_usec;
    double realTimeTakenSeconds = secondsElapsed + uSecondsElapsed/(1000000);
    std::cout<<"\n\nThis is timer final:  "<<realTimeTakenSeconds<<"\n\n";
    std::cout<<"\n\nThis is size of allGets:   "<<allGetsSize<<"\n\n";
  }




  void runMultiUnLoad(int numTimesLoad)
  {
    //declare reader
    AsyncCraqSet myReader1(NULL,NULL);


    //declare craq args
    std::vector<CraqInitializeArgs> craqArgs1;
    CraqInitializeArgs cInitArgs;
    cInitArgs.ipAdd   =     WHICH_SERVER;
    cInitArgs.port    =       WHICH_PORT;



    craqArgs1.push_back(cInitArgs);

    std::vector<CraqOperationResult*> allGets;
    std::vector<CraqOperationResult*> allTrackedSets;


    std::cout<<"\n\nAbout to perform reader initialization.\n\n";
    myReader1.initialize(craqArgs1);


    for(int s=0; s < 10000; ++s)
    {
      iteratedWait(1,&myReader1,allGets, allTrackedSets);
      //    iteratedWait(1,&myReader2,allGets, allTrackedSets);
    }


    std::cout<<"\n\nAbout to perform first set of sets\n\n";
    //performing first sets
    CraqDataKey cDK;
    std::string query;



    struct timeval first_time, finished_time;
    gettimeofday(&first_time,NULL);



    for(int s=0; s < 100000; ++s)
    {
      iteratedWait(1,&myReader1,allGets, allTrackedSets);
      iteratedWait(1,&myReader1,allGets, allTrackedSets);
    }

    //about to perform gets
    std::cout<<"\n\nAbout to perform a bunch of gets\n\n";

    for (int a = 0; a < numTimesLoad; ++a)
    {
      for (int s=numInit; s < numInit+ numPuts; ++s)
      {
        query = "";
        std::stringstream ss;
        ss << s;
        std::string tmper = ss.str();

        for (int t=0; t< CRAQ_DATA_KEY_SIZE - (((int)tmper.size()) + 1); ++t)
        {
          query.append("0");
        }
        query.append(tmper);


        CraqDataSetGet cdSetGet2 (query,s,false,CraqDataSetGet::GET);
        myReader1.get(cdSetGet2);

        iteratedWait(1,&myReader1,allGets,allTrackedSets);

      }
    }



    std::cout<<"\n\nAbout to go through queue iterations\n\n";

    while(myReader1.queueSize() != 0)
    {
      iteratedWait(10000,&myReader1,allGets, allTrackedSets);
      std::cout<<"\n\nThis is queue size:  "<<myReader1.queueSize()<<"\n\n";
      std::cout.flush();
    }

    iteratedWait(70000,&myReader1,allGets, allTrackedSets);

    int allGetsSize = (int) allGets.size();
    std::cout<<"\n\nThis is size of allGets:   "<<allGets.size()<<"\n\n";


    for (int s=0; s < (int)allGets.size(); ++ s)
    {
      printResponse(allGets[s]);
    }

    gettimeofday(&finished_time,NULL);
    double secondsElapsed  = finished_time.tv_sec - first_time.tv_sec;
    double uSecondsElapsed = finished_time.tv_usec - first_time.tv_usec;
    double realTimeTakenSeconds = secondsElapsed + uSecondsElapsed/(1000000);
    std::cout<<"\n\nThis is timer final:  "<<realTimeTakenSeconds<<"\n\n";
    std::cout<<"\n\nThis is size of allGets:   "<<allGetsSize<<"\n\n";
  }



  void printResponse(CraqOperationResult* res)
  {
    std::cout<<"\n\nGot into print response\n\n";

    if (res->whichOperation == CraqOperationResult::GET)
    {
      //    printing a get response
      res->objID[CRAQ_DATA_KEY_SIZE-1] = '\0';
      std::cout<<"GETTING: \n";
      std::cout<<"\tObj ID:   "<<res->objID<<"      Server ID:  "<<res->servID<<"\n";
      if (!res->succeeded)
      {
        std::cout<<"\n\nWARNING.  WARNING.  FAIL.  FAIL.\n\n\n\n";
        exit(1);
      }
    }
    //  if(res.whichOperation == CraqOperationResult::SET)
    else
    {
      res->objID[CRAQ_DATA_KEY_SIZE-1] = '\0';
      std::cout<<"SETTING: \n";
      std::cout<<"\tObj ID:   "<<res->objID<<"      Server ID:  "<<res->servID<<"\n";
      if (!res->succeeded)
      {
        std::cout<<"\n\nWARNING.  WARNING.  FAIL.  FAIL.\n\n\n\n";
        exit(1);
      }
    }
  }


  void iteratedWait(int numWaits,AsyncCraqSet* myReader,std::vector<CraqOperationResult*> &allGetResults,std::vector<CraqOperationResult*>&allTrackedResults)
  {
    for (int s=0; s < numWaits; ++s)
    {
      basicWait(myReader,allGetResults,allTrackedResults);
    }
  }


  void basicWait(AsyncCraqSet* myReader,std::vector<CraqOperationResult*> &allGetResults,std::vector<CraqOperationResult*>&allTrackedResults)
  {
    std::vector<CraqOperationResult*> getResults;
    std::vector<CraqOperationResult*> trackedSetResults;

    myReader -> tick(getResults,trackedSetResults);

    allGetResults.insert(allGetResults.end(), getResults.begin(), getResults.end());
    allTrackedResults.insert(allTrackedResults.end(), trackedSetResults.begin(), trackedSetResults.end());
  }

}//end namespace

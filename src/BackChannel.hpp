
#ifndef ___CBR_BACKCHANNEL_HPP___
#define ___CBR_BACKCHANNEL_HPP___


#include <iostream>
#include <iomanip>


namespace CBR
{


  struct ServerInformation
  {
    ServerID sID;
    std::string lookupName;
  };
  
  
  class SingleServerBackConn
  {
    //this is actually supposed to be a boost tcp connection.
    
  };
  
  
  class BackChannel
  {

  private:
    typedef std::map<ServerID, SingleServerBackConn>  IDConnMap;
    IDConnMap mIDConns;
    ServerID mID;
    Timer mTimer;
    uint64 mTrackNum;


    //a list of logged latencies for each message type.
    typedef std::vector<int> MillisecondLatencyVec
    typedef std::map<BackChannelMessageType, MillisecondLatencyVec>  LatencyStats;
    LatencyStats mLatencies;


    //outstanding querries.
    typedef std::map<uint64,int > TrackNumberDepartTimeMap;
       //the uint64 is the tracking number used to index into the int... millisecond departure time.
    TrackNumberDepartTimeMap mOutstandingQuerries;
    void createConnectionMaps(otherServers); //begins connecting to alternate servers
    
  public:
    BackChannel(ServerID myID, std::vector<ServerInformation> otherServers); //also performs connections
    //    uint64 getTrackNum(); //gives a tracking number
    
    uint64 addLogFor(BackChannelMessageType);//wait for a backchannel message 

    void service();

    void sendAck(ServerID dest_server, const UUID& obj_id, int trackNum);

    ForwardingMessage* generateForwardingMessage(const UUID& obj_id);

    void service();
  };
  
}


#endif

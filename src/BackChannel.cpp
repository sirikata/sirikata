#include "BackChannel.hpp"


//here's what forwarding message looks like




BackChannel::BackChannel(ServerID myID, std::vector<ServerInformation> otherServers)
{
  mTrackNum = 0;
  mTimer.start();
  mID = myID;
  createConnectionMaps(otherServers);
}

uint64 BackChannel::addLogFor(BackChannelMessageType bcmt)
{
  ++mTrackNum;
  Duration dur = mTimer.elapsed();
  int msDur = 
  
  LatencyStats[bcmt]
  
  return mTrackNum;
}


  
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

    
    
  public:
    BackChannel(ServerID myID, std::vector<ServerInformation> otherServers); //
    void initialize();//begins connecting to alternate servers.
    uint64 getTrackNum(); //gives a tracking number
    
    void addLogFor(uint64 tracker, BackChannelMessageType);//wait for a backchannel message 

    void service();

    void sendAck(ServerID dest_server, const UUID& obj_id, int trackNum);

    ForwardingMessage* generateForwardingMessage(const UUID& obj_id);

    void service();
  };

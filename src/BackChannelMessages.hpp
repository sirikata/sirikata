
#ifndef ___CBR_BACK_CHANNEL_MESSAGES_HPP___
#define ___CBR_BACK_CHANNEL_MESSAGES_HPP___



namespace CBR
{

#define BACK_CHANNEL_MESSAGE_TYPE_FORWARDED  1
  typedef uint8 BackChannelMessageType;



  class BackChannelMessage
  {
    public:
    virtual ~BackChannelMessage();
    virtual BackChannelMessageType type() const = 0;

  }; // class Message




  class BackChannelForwarderMessage : pulbic BackChannelMessage
  {
  public:
    BackChannelForwarderMessage(ServerID sID, int trackNumer);
    ~BackChannelForwarderMessage();
    BackChannelMessageType type();

  private:
    ServerID mID;
    uint64 mTrack;
  };
  


}


#endif

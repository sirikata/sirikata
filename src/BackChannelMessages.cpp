
#include "BackChannelMessage.hpp"




BackChannelForwarderMessage::BackChannelForwarderMessage(ServerID sID, int trackingID)
{
  mID = sID;
  mTrack = trackingID;
}


BackChannelForwarderMessage::~BackChannelForwarderMessage()
{

}

BackChannelMessageType BackChannelForwarderMessage::type()
{
  return BACK_CHANNEL_MESSAGE_TYPE_FORWARDED;
}


#ifndef _CBR_FORWARDER_UTILITY_CLASSES_HPP_
#define _CBR_FORWARDER_UTILITY_CLASSES_HPP_



#include "Utility.hpp"


#include "Network.hpp"
#include "ServerNetwork.hpp"





namespace CBR
{

  class Object;


  typedef std::map<UUID, Object*> ObjectMap;

  struct SelfMessage
  {
    SelfMessage(const Network::Chunk& d, bool f)
      : data(d), forwarded(f) {}

    Network::Chunk data;
    bool forwarded;
  };

  //std::deque<SelfMessage> mSelfMessages;

  struct OutgoingMessage {
      OutgoingMessage(const Network::Chunk& _data, const ServerID& _dest, UniqueMessageID uniq)
       : data(_data), dest(_dest), unique(uniq) {}
    size_t size()const{
        return data.size();
    }
    Network::Chunk data;
    ServerID dest;
      UniqueMessageID unique;
  };
  //std::deque<OutgoingMessage> mOutgoingMessages;

}


#endif

#ifndef _CBR_FORWARDER_UTILITY_CLASSES_HPP_
#define _CBR_FORWARDER_UTILITY_CLASSES_HPP_



#include "Utility.hpp"


#include "Network.hpp"
#include "ServerNetwork.hpp"





namespace CBR
{
  class ServerProtocolMessagePair;
  struct ObjMessQBeginSend
  {
      UUID dest_uuid;
    ServerProtocolMessagePair* data;
  };

  class Object;


  typedef std::map<UUID, Object*> ObjectMap;

  struct SelfMessage
  {
    SelfMessage(Message* m, bool f)
      : msg(m), forwarded(f) {}

      Message* msg;
    bool forwarded;
  };

  //std::deque<SelfMessage> mSelfMessages;

  class OSegLookupQueue {
      class ObjMessQBeginSendList : protected std::vector<ObjMessQBeginSend>{
          size_t mTotalSize;
      public:
          size_t ByteSize() const{
              return mTotalSize;
          }
          size_t size()const {
              return std::vector<ObjMessQBeginSend>::size();
          }
          ObjMessQBeginSend&operator[] (size_t where){
              return std::vector<ObjMessQBeginSend>::operator[](where);
          }
          void push_back(const ObjMessQBeginSend&msg) ;//in Forwarder.cpp
          std::vector<ObjMessQBeginSend>::iterator begin(){
              return std::vector<ObjMessQBeginSend>::begin();
          }
          std::vector<ObjMessQBeginSend>::iterator end(){
              return std::vector<ObjMessQBeginSend>::end();
          }
      };
      size_t mTotalSize;
      ObjMessQBeginSendList mNop;
  public:
      typedef std::tr1::unordered_map<UUID,ObjMessQBeginSendList,UUID::Hasher> ObjectMap;
      typedef std::tr1::function<bool(UUID, size_t, size_t)> PushPredicate;
  private:
      ObjectMap mObjects;
      PushPredicate mPredicate;
  public:
      OSegLookupQueue(const PushPredicate&pred) :mPredicate(pred){

      }
      ObjMessQBeginSendList&operator[](const UUID&objid) {
          return mObjects[objid];
      }
      void push(UUID objid,const ObjMessQBeginSend &dat);
      ObjectMap::iterator find(const UUID&objid) {
          return mObjects.find(objid);
      }
      ObjectMap::iterator end() {
          return mObjects.end();
      }
      void erase(ObjectMap::iterator where) {
          mTotalSize-=where->second.ByteSize();
          mObjects.erase(where);
      }
  };
}


#endif

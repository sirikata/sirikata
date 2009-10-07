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
    SelfMessage(Message* m, bool f)
      : msg(m), forwarded(f) {}

      Message* msg;
    bool forwarded;
  };

  //std::deque<SelfMessage> mSelfMessages;

  class OSegLookupQueue {
      class ObjMessQBeginSendList : protected std::vector<CBR::Protocol::Object::ObjectMessage*>{
          typedef std::vector<CBR::Protocol::Object::ObjectMessage*> ObjectMessageVector;
          size_t mTotalSize;
      public:
          size_t ByteSize() const{
              return mTotalSize;
          }
          size_t size()const {
              return ObjectMessageVector::size();
          }
          CBR::Protocol::Object::ObjectMessage*& operator[] (size_t where){
              return ObjectMessageVector::operator[](where);
          }
          void push_back(CBR::Protocol::Object::ObjectMessage* msg) ;//in Forwarder.cpp
          ObjectMessageVector::iterator begin(){
              return ObjectMessageVector::begin();
          }
          ObjectMessageVector::iterator end(){
              return ObjectMessageVector::end();
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
      void push(UUID objid, CBR::Protocol::Object::ObjectMessage* dat);
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

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


/** The data used or stored during an OSeg lookup. Strictly speaking,
 *  only the message itself is required, but we maintain additional
 *  information for bookkeeping and statistics purposes.
 */
struct OSegLookup
{
    bool forward;
    CBR::Protocol::Object::ObjectMessage* msg;
};

  class OSegLookupQueue {
      class ObjMessQBeginSendList : protected std::vector<OSegLookup>{
          typedef std::vector<OSegLookup> OSegLookupVector;
          size_t mTotalSize;
      public:
          size_t ByteSize() const{
              return mTotalSize;
          }
          size_t size()const {
              return OSegLookupVector::size();
          }
          OSegLookup& operator[] (size_t where){
              return OSegLookupVector::operator[](where);
          }
          void push_back(const OSegLookup& msg) ;//in Forwarder.cpp
          OSegLookupVector::iterator begin(){
              return OSegLookupVector::begin();
          }
          OSegLookupVector::iterator end(){
              return OSegLookupVector::end();
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
      void push(UUID objid, const OSegLookup& lu);
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

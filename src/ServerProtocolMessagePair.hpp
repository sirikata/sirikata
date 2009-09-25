#ifndef _SERVERPRTOTOBOCLMETEHPAIR_HPP
#define _SERVERPRTOTOBOCLMETEHPAIR_HPP
namespace CBR {
    // Note: only public because the templated version of FairObjectMessageQueue requires it

class ServerProtocolMessagePair {
    private:
    
    std::pair<ServerID,ObjectMessage> mPair;
    UniqueMessageID mID;
public:
    ServerProtocolMessagePair(ObjectMessage&msg):mPair(0,msg),mID(0){}
    ServerProtocolMessagePair(const ServerID&sid, const ObjectMessage&data,UniqueMessageID id):mPair(sid,data),mID(id){}
    unsigned int size()const {
        return mPair.second.size();
    }
    bool empty() const {
        return size()==0;
    }
    ServerID dest() const {
        return mPair.first;
    }
    
    ServerID dest(ServerID newval) {
        mPair.first = newval;
        return mPair.first;
    }
    
    const ObjectMessage& data() const {
        return mPair.second;
    }
    ObjectMessage& data() {
        return mPair.second;
    }
    
    UniqueMessageID id() const {
        return mID;
    }
};


}
#endif

#ifndef _CBR_SERVERMESSAGEPAIR
#define _CBR_SERVERMESSAGEPAIR
namespace CBR {
    class ServerMessagePair {
    private:
        std::pair<ServerID,Network::Chunk> mPair;
    public:
        ServerMessagePair(const ServerID&sid, const Network::Chunk&data):mPair(sid,data){}

        explicit ServerMessagePair(size_t size):mPair(0,Network::Chunk(size)){

        }
        unsigned int size()const {
            return mPair.second.size();
        }
        bool empty() const {
            return size()==0;
        }
        ServerID dest() const {
            return mPair.first;
        }

        // FIXME does changing this to return a reference affect anything? this is quite wasteful as is
        const Network::Chunk data() const {
            return mPair.second;
        }
    };
}

#endif

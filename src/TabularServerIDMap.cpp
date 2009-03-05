#include "Network.hpp"
#include "TabularServerIDMap.hpp"

namespace CBR {


TabularServerIDMap::TabularServerIDMap(std::istream&filestream) {
    int count=1;
    while(!filestream.bad()&&!filestream.fail()&&!filestream.eof()) {
        char ip[1025];
        ip[1024]='\0';
        char service[1025];
        service[1024]='\0';
        filestream.getline(ip,1024,':');
        if (filestream.eof()) break;
        filestream.getline(service,1024,'\n');
        Address4 addy(Sirikata::Network::Address(ip,service));
        mIDMap[count]=addy;
        mAddressMap[addy]=count;
        count++;
    }
}
ServerID *TabularServerIDMap::lookup(const Address4& address){
    if (mAddressMap.find(address)!=mAddressMap.end())
        return &mAddressMap.find(address)->second;
    return NULL;
}
Address4 *TabularServerIDMap::lookup(const ServerID& server_id){
    if (mIDMap.find(server_id)!=mIDMap.end())
        return &mIDMap.find(server_id)->second;
    return NULL;
}
}

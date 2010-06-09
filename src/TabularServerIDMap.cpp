#include "Address4.hpp"
#include "TabularServerIDMap.hpp"

namespace Sirikata {


TabularServerIDMap::TabularServerIDMap(std::istream&filestream) {
    int count=1;
    while(!filestream.bad() && !filestream.fail() && !filestream.eof()) {
        char ip[1025];
        ip[1024]='\0';
        char service[1025];
        service[1024]='\0';
        char ohservice[1025];
        ohservice[1024]='\0';
        filestream.getline(ip,1024,':');
        if (filestream.eof()) break;
        filestream.getline(service,1024,':');
        if (filestream.eof()) break;
        filestream.getline(ohservice,1024,'\n');

        Address4 internal_addy(Sirikata::Network::Address(ip,service));
        mInternalIDMap[count] = internal_addy;
        mInternalAddressMap[internal_addy] = count;

        Address4 external_addy(Sirikata::Network::Address(ip,ohservice));
        mExternalIDMap[count] = external_addy;
        mExternalAddressMap[external_addy] = count;

	count++;
    }
}

ServerID *TabularServerIDMap::lookupInternal(const Address4& address){
    if (mInternalAddressMap.find(address)!=mInternalAddressMap.end())
        return &mInternalAddressMap.find(address)->second;
    return NULL;
}
Address4 *TabularServerIDMap::lookupInternal(const ServerID& server_id){
    if (mInternalIDMap.find(server_id)!=mInternalIDMap.end())
        return &mInternalIDMap.find(server_id)->second;
    return NULL;
}

ServerID *TabularServerIDMap::lookupExternal(const Address4& address){
    if (mExternalAddressMap.find(address)!=mExternalAddressMap.end())
        return &mExternalAddressMap.find(address)->second;
    return NULL;
}
Address4 *TabularServerIDMap::lookupExternal(const ServerID& server_id){
    if (mExternalIDMap.find(server_id)!=mExternalIDMap.end())
        return &mExternalIDMap.find(server_id)->second;
    return NULL;
}

}

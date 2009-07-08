#include <oh/Platform.hpp>
#include <util/SpaceID.hpp>
#include <network/Address.hpp>
#include "oh/SpaceIDMap.hpp"

namespace Sirikata {

using namespace Network;

SpaceIDMap::SpaceIDMap(const String&options) {
    String tempOptions;
    String::size_type where=tempOptions.find(",");
    do{
        String option=tempOptions.substr(0,where);
        String::size_type equa=option.find("=");
        if (equa!=String::npos) {
            try {
                insert(SpaceID(UUID(option.substr(0,equa),UUID::HumanReadable())),
                       Address::lexical_cast(option.substr(equa+1)).as<Address>());
            }catch (std::invalid_argument&) {
                SILOG(oh,error,"Cannot parse "<<option<<" as <uuid>=<ip address>");
            }
        }
    }while(where!=String::npos&&!(tempOptions=tempOptions.substr(where+1)).empty());
}

void SpaceIDMap::insert(const SpaceID&id, const Address&addr){
    mCache.insert(IDAddressMap::value_type(id, addr));
}
void SpaceIDMap::lookup(const SpaceID&id, const std::tr1::function<void(const Address*)>&cb){
    IDAddressMap::iterator where=mCache.find(id);
    if (where!=mCache.end()) {
        cb(&where->second);
    }else{
        cb(NULL);
    }
}


}

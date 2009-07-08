#include <oh/Platform.hpp>
#ifndef _SPACEIDMAP_HPP_
#define _SPACEIDMAP_HPP_
namespace Sirikata {
class SIRIKATA_OH_EXPORT SpaceIDMap {
    typedef std::tr1::unordered_map<SpaceID,Network::Address,SpaceID::Hasher> IDAddressMap;
    IDAddressMap mCache;
  public:
    SpaceIDMap() {}
    SpaceIDMap(const String&spaceIDMapOptions);
    void insert(const SpaceID&id, const Network::Address&addr);
    void lookup(const SpaceID&id, const std::tr1::function<void(const Network::Address*)>&);
    ~SpaceIDMap(){}
};

}
#endif

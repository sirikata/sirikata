#include <oh/Platform.hpp>
#ifndef _SPACEIDMAP_HPP_
#define _SPACEIDMAP_HPP_
namespace Sirikata {
class SIRIKATA_OH_EXPORT SpaceIDMap {
    std::tr1::unordered_map<SpaceID,Network::Address,SpaceID::Hasher> mCache;
  public:
    SpaceIDMap(const String&spaceIDMapOptions);
    virtual void lookup(const SpaceID&id, const std::tr1::function<void(const Network::Address*)>&);
    virtual ~SpaceIDMap(){}
};

}
#endif

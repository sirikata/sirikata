#include <oh/Platform.hpp>
#include <util/SpaceID.hpp>
#include "oh/SpaceConnection.hpp"
#include "oh/TopLevelSpaceConnection.hpp"
namespace Sirikata {
size_t SpaceConnection::Hasher::operator() (const SpaceConnection&sc) const{
            return SpaceID::Hasher()(sc.mTopLevelStream->id());
        }
}

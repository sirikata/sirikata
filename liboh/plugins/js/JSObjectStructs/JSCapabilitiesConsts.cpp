
#include "JSCapabilitiesConsts.hpp"
#include <sirikata/core/util/Platform.hpp>
#include <sirikata/oh/Platform.hpp>

namespace Sirikata{
namespace JS{

uint32 Capabilities::getFullCapabilities()
{
    return SEND_MESSAGE|RECEIVE_MESSAGE|IMPORT|CREATE_PRESENCE|CREATE_ENTITY|EVAL|PROX_CALLBACKS|PROX_QUERIES|CREATE_SANDBOX|GUI|HTTP;
}

} //end namespace js
} //end namespace sirikata.

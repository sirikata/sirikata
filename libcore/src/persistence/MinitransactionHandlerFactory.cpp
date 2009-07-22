#include <util/Platform.hpp>
#include "util/Factory.hpp"
#include <persistence/MinitransactionHandlerFactory.hpp>


AUTO_SINGLETON_INSTANCE(Sirikata::MinitransactionHandlerFactory);

namespace Sirikata {

MinitransactionHandlerFactory&MinitransactionHandlerFactory::getSingleton(){
    return AutoSingleton<MinitransactionHandlerFactory>::getSingleton();
}
void MinitransactionHandlerFactory::destroy(){
    AutoSingleton<MinitransactionHandlerFactory>::destroy();
}


}

#include <sirikata/core/util/Standard.hh>

#include <sirikata/core/transfer/TransferMediator.hpp>

AUTO_SINGLETON_INSTANCE(Sirikata::Transfer::TransferMediator);


using namespace Sirikata;
using namespace Sirikata::Transfer;

namespace Sirikata{
namespace Transfer{

TransferMediator& TransferMediator::getSingleton() {
    return AutoSingleton<TransferMediator>::getSingleton();
}
void TransferMediator::destroy() {
    AutoSingleton<TransferMediator>::destroy();
}

}
}
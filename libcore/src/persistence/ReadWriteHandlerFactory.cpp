#include <util/Platform.hpp>
#include "util/Factory.hpp"
#include <persistence/ReadWriteHandlerFactory.hpp>


AUTO_SINGLETON_INSTANCE(Sirikata::ReadWriteHandlerFactory);


namespace Sirikata {
ReadWriteHandlerFactory&ReadWriteHandlerFactory::getSingleton(){
    return AutoSingleton<ReadWriteHandlerFactory>::getSingleton();
}
void ReadWriteHandlerFactory::destroy(){
    AutoSingleton<ReadWriteHandlerFactory>::destroy();
}
ReadWriteHandlerFactory::ReadWriteHandlerFactory(){

}
ReadWriteHandlerFactory::~ReadWriteHandlerFactory(){

}


}

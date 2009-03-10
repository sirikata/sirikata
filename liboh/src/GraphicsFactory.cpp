#include <util/Platform.hpp>
#include "GraphicsFactory.hpp"

AUTO_SINGLETON_INSTANCE(Sirikata::GraphicsFactory);
namespace Sirikata {
GraphicsFactory& GraphicsFactory::getSingleton() {
    return AutoSingleton<GraphicsFactory>::getSingleton();
}
void GraphicsFactory::destroy() {
	AutoSingleton<GraphicsFactory>::destroy();
}
}

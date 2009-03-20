#include <util/Platform.hpp>
#include <oh/SimulationFactory.hpp>
AUTO_SINGLETON_INSTANCE(Sirikata::SimulationFactory);
namespace Sirikata {
SimulationFactory& SimulationFactory::getSingleton() {
    return AutoSingleton<SimulationFactory>::getSingleton();
}
void SimulationFactory::destroy() {
	AutoSingleton<SimulationFactory>::destroy();
}
}

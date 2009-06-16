#include <util/Platform.hpp>
#include <oh/SimulationFactory.hpp>
AUTO_SINGLETON_INSTANCE(Sirikata::SimulationFactory);
namespace Sirikata {
SimulationFactory& SimulationFactory::getSingleton() {
    std::cout << "dbm: about to call getSingleton" << std::endl;
    SimulationFactory& temp = AutoSingleton<SimulationFactory>::getSingleton();
    std::cout << "dbm: returned from getSingleton" << std::endl;
    return temp;
}
void SimulationFactory::destroy() {
	AutoSingleton<SimulationFactory>::destroy();
}
}

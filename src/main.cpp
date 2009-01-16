#include "options/Options.hpp"
namespace Iridium {
//InitializeOptions main_options("verbose",

}

int main(int argc,const char**argv) {
    using namespace Iridium;
    OptionSet::getOptions("")->parse(argc,argv);
    return 0;
}

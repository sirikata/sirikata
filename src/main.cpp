#include "options/Options.hpp"
namespace Sirikata {
//InitializeOptions main_options("verbose",

}

int main(int argc,const char**argv) {
    using namespace Sirikata;
    OptionSet::getOptions("")->parse(argc,argv);
    return 0;
}

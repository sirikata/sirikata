
#include "../JSObjectScriptManager.hpp"
#include "../JSObjectScript.hpp"
#include <string>



int main (int argc, char** argv)
{
    //for now, just interpret the first argument as the file to execute.

    if (argc!= 2)
    {
        std::cout<<"\n\nError.  First argument should be the filename for an emerson script to run headlessly.  Aborting.\n";
        return 0;
    }

    Sirikata::JS::JSObjectScriptManager jsman(NULL, "");
    std::string fileToCheck(argv[1]);

    std::string args = "";
    std::string script ( "system.import('" + fileToCheck+ "');");
    Sirikata::JS::JSObjectScript* jsobj = jsman.createHeadless(args, script,10000000);

    delete jsobj;
    return 0;
}

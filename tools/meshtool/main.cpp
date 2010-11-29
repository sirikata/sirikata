/*  Sirikata
 *  main.cpp
 *
 *  Copyright (c) 2010, Ewen Cheslack-Postava
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Sirikata nor the names of its contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sirikata/core/util/PluginManager.hpp>
#include <sirikata/mesh/Filter.hpp>

void usage() {
    printf("Usage: meshtool [-h, --help] [--list] --filter1 --filter2=filter,options\n");
    printf("   --help will print this help message\n");
    printf("   --list will print the list of filters\n");
    printf(" Example: meshtool --load=/path/to/file.dae\n");
}

int main(int argc, char** argv) {
    using namespace Sirikata;
    using namespace Sirikata::Mesh;

    // Check for help request
    for(int argi = 1; argi < argc; argi++) {
        std::string arg_str(argv[argi]);
        if (arg_str == "-h" || arg_str == "--help") {
            usage();
            return 0;
        }
    }

    PluginManager plugins;
    plugins.loadList("colladamodels");
    plugins.loadList("common-filters");
    plugins.loadList("nvtt");

    //Check for list request
    for(int argi = 1; argi < argc; argi++) {
        std::string arg_str(argv[argi]);
        if (arg_str == "--list") {
            std::list<std::string> filterList = FilterFactory::getSingleton().getNames();
            printf("meshtool: printing filter list:\n");
            for(std::list<std::string>::const_iterator it = filterList.begin(); it != filterList.end(); it++) {
                printf("%s\n", it->c_str());
            }
            return 0;
        }
    }

    FilterDataPtr current_data(new FilterData);
    for(int argi = 1; argi < argc; argi++) {
        std::string arg_str(argv[argi]);
        if (arg_str.substr(0, 2) != "--") {
            std::cout << "Couldn't parse argument: " << arg_str << std::endl;
            exit(-1);
        }
        arg_str = arg_str.substr(2);
        // Split filter name and args
        std::string filter_name, filter_args;
        int equal_idx = arg_str.find('=');
        if (equal_idx != std::string::npos) {
            filter_name = arg_str.substr(0, equal_idx);
            filter_args = arg_str.substr(equal_idx+1);
        }
        else {
            filter_name = arg_str;
            filter_args = "";
        }
        // Verify
        if (!FilterFactory::getSingleton().hasConstructor(filter_name)) {
            std::cout << "Couldn't find filter: " << filter_name << std::endl;
            exit(-1);
        }
        // And apply
        Filter* filter = FilterFactory::getSingleton().getConstructor(filter_name)(filter_args);
        current_data = filter->apply(current_data);
        delete filter;
    }

    return 0;
}

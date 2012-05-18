/*  Sirikata
 *  main.cpp
 *
 *  Copyright (c) 2012, Jeff Terrace
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

#include <sirikata/core/options/Options.hpp>
#include <sirikata/core/options/CommonOptions.hpp>
#include <sirikata/core/util/PluginManager.hpp>
#include <iostream>

int main(int argc, char** argv) {
    using namespace Sirikata;

    InitOptions();

    PluginManager plugins;
        plugins.loadList("colladamodels");
        plugins.loadList("mesh-billboard");
        plugins.loadList("common-filters");
        plugins.loadList("nvtt");

    const OptionSet::NamedOptionSetMapPtr m = OptionSet::optionSets();

    for (OptionSet::NamedOptionSetMap::const_iterator setit = m->begin(); setit != m->end(); setit++) {
        std::cout << "NameSpace (" << setit->first.getString() << ")" << std::endl;
        const OptionSet::OptionNameMap& nameMap = setit->second->getOptionsMap();
        for (OptionSet::OptionNameMap::const_iterator optionit = nameMap.begin(); optionit != nameMap.end(); optionit++) {
            std::string typeName = optionit->second->typeName() == NULL ? "Unknown" : optionit->second->typeName();
            std::cout << "  " << optionit->first << " (" << typeName << ")" << std::endl;
            std::cout << "    " << optionit->second->description() << std::endl;
        }
        std::cout << std::endl;
    }

    return 0;
}

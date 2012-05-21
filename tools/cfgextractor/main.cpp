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
#include <json_spirit/writer.h>
#include <json_spirit/value.h>
#include <iostream>

/*
 * The name() method of std::type_info returns a non-standard value. On gnu
 * platforms, it is a mangled name, so the internal cxa_demangle function can
 * be used. This probably doesn't exist on other platforms, so the ifdef checks
 * to see if the platform is GNU first. This is not guaranteed to work properly
 * but it seems to work.
 *
 * The method to demangle is taken from http://stackoverflow.com/q/281818/624900
 */
#ifdef __GNUG__
#include <cxxabi.h>
const std::string demangle(const char* name) {
    int status = -4;
    char* res = abi::__cxa_demangle(name, NULL, NULL, &status);
    const char* const demangled_name = (status == 0) ? res : name;
    std::string ret_val(demangled_name);
    free(res);
    return ret_val;
}
#else
const std::string demangle(const char* name) {
    return name;
}
#endif

typedef json_spirit::Object Object;

int main(int argc, char** argv) {
    using namespace Sirikata;

    InitOptions();

    PluginManager plugins;

    Object allNamespaces;
    const OptionSet::NamedOptionSetMapPtr m = OptionSet::optionSets();

    for (OptionSet::NamedOptionSetMap::const_iterator setit = m->begin(); setit != m->end(); setit++) {

        //std::cout << "NameSpace (" << setit->first.getString() << ")" << std::endl;
        Object namespaceObj;
        const OptionSet::OptionNameMap& nameMap = setit->second->getOptionsMap();

        for (OptionSet::OptionNameMap::const_iterator optionit = nameMap.begin(); optionit != nameMap.end(); optionit++) {
            std::string typeName = optionit->second->typeName() == NULL ? "Unknown" : demangle(optionit->second->typeName());

            Object configObj;
            configObj["type"] = typeName;
            configObj["description"] = optionit->second->description();
            configObj["default"] = optionit->second->defaultValue();
            namespaceObj[optionit->first] = configObj;

            //std::cout << "  --" << optionit->first << " (" << typeName << ")" << std::endl;
            //std::cout << "    " << optionit->second->description() << std::endl;
            //std::cout << "    Default: \"" << optionit->second->defaultValue() << "\"" << std::endl;
        }

        allNamespaces[setit->first.getString()] = namespaceObj;
        //std::cout << std::endl;
    }

    json_spirit::write(allNamespaces, std::cout, json_spirit::pretty_print);
    std::cout << std::endl;

    return 0;
}

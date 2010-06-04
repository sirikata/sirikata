/*  Sirikata cppoh -- Object Host
 *  Config.hpp
 *
 *  Copyright (c) 2009, Patrick Reiter Horn
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
#ifndef SIMPLE_CONFIG_HPP_
#define SIMPLE_CONFIG_HPP_

namespace Sirikata {

class OptionMap;
typedef std::tr1::shared_ptr<OptionMap> OptionMapPtr;

struct OptionDoesNotExist : public std::runtime_error {
    OptionDoesNotExist(const std::string &optionName)
        : std::runtime_error("Option '" + optionName + "' does not exist!") {
    }
};

class OptionMap : public std::map<std::string, OptionMapPtr>{
private:
    std::string value;
public:
    OptionMap(std::string value) : value(value) {
    }
    OptionMap() {}

    const std::string & getValue() const {
        return value;
    }
    const OptionMapPtr &put(const std::string &key, OptionMapPtr value) {
        iterator iter = insert(OptionMap::value_type(key, value)).first;
        return (*iter).second;
    }
    const OptionMapPtr &get (const std::string &key) const {
        static OptionMapPtr nulloptionmapptr;
        const_iterator iter = find(key);
        if (iter != end()) {
            return (*iter).second;
        } else {
            return nulloptionmapptr;
        }
    }
    OptionMap &operator[] (const std::string &key) const {
        const OptionMapPtr &ptr = get(key);
        if (!ptr) {
            throw OptionDoesNotExist(key);
        }
        return *ptr;
    }
};
// Can be used to dump from file.
SIRIKATA_EXPORT std::ostream &operator<< (std::ostream &os, const OptionMap &om);

SIRIKATA_EXPORT void parseConfig(
        const std::string &input,
        const OptionMapPtr &globalvariables, //< Identical to output
        const OptionMapPtr &options, //< output
        std::string::size_type &pos //< reference to variable initialized to 0.
);

}
#endif

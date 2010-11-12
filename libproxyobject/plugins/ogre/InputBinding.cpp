/*  Sirikata libproxyobject -- Ogre Graphics Plugin
 *  InputBinding.cpp
 *
 *  Copyright (c) 2009, Ewen Cheslack-Postava
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

#include "InputBinding.hpp"

#include <boost/program_options.hpp>

namespace Sirikata {
namespace Graphics {

using namespace Input;

InputBinding::InputBinding() {
}

InputBinding::~InputBinding() {
}

void InputBinding::add(const InputBindingEvent& evt, InputResponse* response) {
    InputResponse::InputEventDescriptorList evts = response->getInputEvents(evt);

    for(InputResponse::InputEventDescriptorList::iterator it = evts.begin(); it != evts.end(); it++)
        mResponses[*it] = response;
}

void InputBinding::addFromFile(const String& filename, InputResponseMap responses) {
    // Empty options, all the options will be generated automatically
    boost::program_options::options_description opts_desc;
    // Parse the file, allowing unknown options
    std::ifstream fp(filename.c_str());
    boost::program_options::parsed_options parsed_opts =
        boost::program_options::parse_config_file(fp, opts_desc, true);
    for(int i = 0; i < parsed_opts.options.size(); i++) {
        assert(parsed_opts.options[i].value.size() == 1);
        InputBindingEvent ibe = InputBindingEvent::fromString(parsed_opts.options[i].string_key);
        if (!ibe.valid()) {
            SILOG(ogre,error,"[OGRE] Invalid input binding input: " << parsed_opts.options[i].string_key);
            continue;
        }
        InputResponseMap::const_iterator resp_it = responses.find(parsed_opts.options[i].value[0]);
        if (resp_it == responses.end()) {
            SILOG(ogre,error,"[OGRE] Invalid input binding response: " << parsed_opts.options[i].value[0]);
            continue;
        }
        add(ibe, resp_it->second);
    }
}

void InputBinding::handle(Input::InputEventPtr& evt) {
    Input::EventDescriptor descriptor = evt->getDescriptor();

    Binding::iterator it = mResponses.find(descriptor);
    if (it != mResponses.end()) {
        InputResponse* response = it->second;
        response->invoke(evt);
    }
}

} // namespace Graphics
} // namespace Sirikata

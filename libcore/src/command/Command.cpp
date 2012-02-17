// Copyright (c) 2012 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include <sirikata/core/util/Standard.hh>
#include <sirikata/core/command/Command.hpp>

namespace Sirikata {
namespace Command {

using namespace boost::property_tree;

String Command::sCommandKey("command");

Command::Command(const CommandKey& cmd) {
    put(sCommandKey, cmd);
}

Command::Command(const boost::property_tree::ptree& orig)
 : ptree(orig)
{
    if (!valid()) throw InvalidCommandException();
}

Command::Command() {
}

bool Command::valid() const {
    return (find(sCommandKey) != not_found());
}


} // namespace Command
} // namespace Sirikata

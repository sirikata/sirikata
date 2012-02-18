// Copyright (c) 2012 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include <sirikata/core/util/Standard.hh>
#include <sirikata/core/command/Command.hpp>

namespace Sirikata {
namespace Command {

using namespace boost::property_tree;

namespace {
String sCommandKey("command");
}

bool CommandIsValid(const Command& cmd) {
    return (cmd.find(sCommandKey) != cmd.not_found());
}

void CommandSetName(Command& cmd, const String& name) {
    cmd.put(sCommandKey, name);
}

} // namespace Command
} // namespace Sirikata

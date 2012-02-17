// Copyright (c) 2012 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include <sirikata/core/util/Standard.hh>
#include <sirikata/core/command/Commander.hpp>

namespace Sirikata {
namespace Command {

Commander::Commander() {
}
Commander::~Commander() {
}

void Commander::registerCommand(const CommandKey& name, CommandHandler handler) {
    Lock lck(mMutex);

    if (mHandlers.find(name) != mHandlers.end())
        throw DuplicateHandlerException();

    mHandlers[name] = handler;
}


void Commander::unregisterCommand(const CommandKey& name) {
    Lock lck(mMutex);

    HandlerMap::iterator it = mHandlers.find(name);
    assert(it != mHandlers.end());
    mHandlers.erase(it);
}

CommandHandler Commander::getHandler(const CommandKey& name) {
    Lock lck(mMutex);

    HandlerMap::iterator it = mHandlers.find(name);
    if (it == mHandlers.end())
        return CommandHandler();
    return mHandlers[name];
}


} // namespace Command
} // namespace Sirikata

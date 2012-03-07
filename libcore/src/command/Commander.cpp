// Copyright (c) 2012 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include <sirikata/core/util/Standard.hh>
#include <sirikata/core/command/Commander.hpp>

AUTO_SINGLETON_INSTANCE(Sirikata::Command::CommanderFactory);

namespace Sirikata {
namespace Command {

Commander::Commander() {
    registerCommand(
        "meta.commands",
        std::tr1::bind(&Commander::commandListCommands, this, _1, _2, _3)
    );
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


void Commander::commandListCommands(const Command& cmd, Commander* cmdr, CommandID cmdid) {
    Result result = EmptyResult();

    result.put( String("commands"), Array());
    Array& commands_ary = result.getArray("commands");
    for(HandlerMap::iterator it = mHandlers.begin(); it != mHandlers.end(); it++)
        commands_ary.push_back(it->first);

    cmdr->result(cmdid, result);
}




CommanderFactory& CommanderFactory::getSingleton() {
    return AutoSingleton<CommanderFactory>::getSingleton();
}

void CommanderFactory::destroy() {
    AutoSingleton<CommanderFactory>::destroy();
}

} // namespace Command
} // namespace Sirikata

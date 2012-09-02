// Copyright (c) 2012 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_LIBPINTOLOC_BASE_PROX_COMMANDABLE_HPP_
#define _SIRIKATA_LIBPINTOLOC_BASE_PROX_COMMANDABLE_HPP_

#include <sirikata/pintoloc/Platform.hpp>
#include <sirikata/core/service/Context.hpp>
#include <sirikata/core/command/Command.hpp>

namespace Sirikata {
namespace Pinto {

/** BaseProxCommandable sets up the basic commands that all prox implementations
 *  should be able to implement and provides some utilities to make implementing
 *  those commands more concise.
 */
class SIRIKATA_LIBPINTOLOC_EXPORT BaseProxCommandable {
public:
    /** Create a BaseProxCommandable. The commands will not be registered until
     *  you call registerBaseProxCommands();
     */
    BaseProxCommandable();
    virtual ~BaseProxCommandable();

    // We use a separate registration because if the class wants to inherit from
    // BaseProxCommandable but also create the IOStrandPtr in its constructor,
    // it needs to be able to register commands after it's initializer list
    // runs.

    /** Register commands with the commander.
     *  \param ctx the Context containing the Commander to register with
     *  \param prefix a prefix for commands, e.g. "space.prox" to get
     *         "space.prox.rebuild" as one of the commands
     */
    void registerBaseProxCommands(Context* ctx, const String& prefix);
    void registerBaseProxCommands(Context* ctx, const String& prefix, Network::IOStrandPtr strand);
    void registerBaseProxCommands(Context* ctx, const String& prefix, Network::IOStrand* strand);

    virtual void commandProperties(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid) = 0;
    virtual void commandListHandlers(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid) = 0;
    virtual void commandListNodes(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid) = 0;
    virtual void commandListQueriers(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid) = 0;
    virtual void commandForceRebuild(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid) = 0;
    virtual void commandStats(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid) = 0;

  private:
    void registerCommands(Context* ctx, const String& prefix, Network::IOStrand* strand);
};

} // namespace Pinto
} // namespace Sirikata

#endif //_SIRIKATA_LIBPINTOLOC_BASE_PROX_COMMANDABLE_HPP_

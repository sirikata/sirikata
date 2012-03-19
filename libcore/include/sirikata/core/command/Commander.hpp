// Copyright (c) 2012 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_LIBCORE_COMMAND_COMMANDER_HPP_
#define _SIRIKATA_LIBCORE_COMMAND_COMMANDER_HPP_

#include <sirikata/core/command/Command.hpp>
#include <sirikata/core/util/Factory.hpp>
#include <sirikata/core/service/Context.hpp>
#include <boost/thread.hpp>

namespace Sirikata {
namespace Command {

/** Commanders accept external commands (e.g. over the network) and dispatch
 *  them to the appropriate handlers.  This provides the basic interface and
 *  implementation for handler registration, dispatch, and result
 *  handling. Implementations are responsible for generating Commands, invoking
 *  dispatch(), and sending out replies.
 */
class SIRIKATA_EXPORT Commander {
public:
    Commander();
    virtual ~Commander();

    /** Register a handler for a command. Throws DuplicateHandlerException if
     *  the handler is already registered.
     *
     *  \param name the name of the command
     *  \param handler the CommandHandler to process this
     *
     *  \note Your handler is responsible for ensuring it executes on the
     *  correct thread. Commanders may invoke them from any thread, especially
     *  since they may have separate networking threads.
     */
    virtual void registerCommand(const CommandKey& name, CommandHandler handler);
    /** Unregister a handler for the command.
     *  \param name the name of the command
     */
    virtual void unregisterCommand(const CommandKey& name);

    /** Return a result from a CommandHandler. This will trigger the process of
     *  returning the result to the requestor.
     */
    virtual void result(CommandID id, const Result& result) = 0;

  protected:
    /** Utility for implementations which looks up the appropriate handler for
     *  the command.
     */
    CommandHandler getHandler(const CommandKey& name);

    // Meta commands -- commands the commander implements to allow getting data about itself
    void commandListCommands(const Command& cmd, Commander* cmdr, CommandID cmdid);

    typedef boost::recursive_mutex Mutex;
    typedef boost::lock_guard<Mutex> Lock;
    Mutex mMutex;

    typedef std::tr1::unordered_map<CommandKey, CommandHandler> HandlerMap;
    HandlerMap mHandlers;
}; // class Commandable


class DuplicateHandlerException : public std::exception {
public:
    DuplicateHandlerException() {}
    virtual ~DuplicateHandlerException() throw() {}

    char const* what() const throw() {
        return "Duplicate handler found during Commander registration";
    }
private:
};


class SIRIKATA_EXPORT CommanderFactory :
        public Factory2<Commander*, Context*, const String&>,
        public AutoSingleton<CommanderFactory>
{
  public:
    static CommanderFactory& getSingleton();
    static void destroy();
};

} // namespace Command
} // namespace Sirikata


#endif //_SIRIKATA_LIBCORE_COMMAND_COMMANDER_HPP_

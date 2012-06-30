// Copyright (c) 2012 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include <sirikata/pintoloc/BaseProxCommandable.hpp>
#include <sirikata/core/command/Commander.hpp>
#include <sirikata/core/network/IOStrandImpl.hpp>

namespace Sirikata {
namespace Pinto {

BaseProxCommandable::BaseProxCommandable() {}

BaseProxCommandable::~BaseProxCommandable() {}

void BaseProxCommandable::registerBaseProxCommands(Context* ctx, const String& prefix) {
    registerCommands(ctx, prefix, NULL);
}

void BaseProxCommandable::registerBaseProxCommands(Context* ctx, const String& prefix, Network::IOStrandPtr strand) {
    registerCommands(ctx, prefix, strand.get());
}

void BaseProxCommandable::registerBaseProxCommands(Context* ctx, const String& prefix, Network::IOStrand* strand) {
    registerCommands(ctx, prefix, strand);
}

void BaseProxCommandable::registerCommands(Context* ctx, const String& prefix, Network::IOStrand* strand) {
    assert(!prefix.empty());

    // Implementations may add more commands, but these should always be
    // available. They get dispatched to the prox strand so implementations only
    // need to worry about processing them.
    if (ctx->commander()) {
        // Get basic properties (both fixed and dynamic debugging
        // state) about this query processor.
        ctx->commander()->registerCommand(
            prefix + ".properties",
            strand
            ? Command::CommandHandler(strand->wrap(std::tr1::bind(&BaseProxCommandable::commandProperties, this, _1, _2, _3)))
            : Command::CommandHandler(std::tr1::bind(&BaseProxCommandable::commandProperties, this, _1, _2, _3))
        );

        // Get a list of the handlers by name and their basic properties. The
        // particular names and properties may be implementation dependent.
        ctx->commander()->registerCommand(
            prefix + ".handlers",
            strand
            ? Command::CommandHandler(strand->wrap(std::tr1::bind(&BaseProxCommandable::commandListHandlers, this, _1, _2, _3)))
            : Command::CommandHandler(std::tr1::bind(&BaseProxCommandable::commandListHandlers, this, _1, _2, _3))
        );

        // Get a list of nodes within one of the handlers. Must specify the
        // handler name as part of the request
        ctx->commander()->registerCommand(
            prefix + ".nodes",
            strand
            ? Command::CommandHandler(strand->wrap(std::tr1::bind(&BaseProxCommandable::commandListNodes, this, _1, _2, _3)))
            : Command::CommandHandler(std::tr1::bind(&BaseProxCommandable::commandListNodes, this, _1, _2, _3))
        );

        // Force a rebuild on one of the handlers. Must specify the handler name
        // as part of the request.
        ctx->commander()->registerCommand(
            prefix + ".rebuild",
            strand
            ? Command::CommandHandler(strand->wrap(std::tr1::bind(&BaseProxCommandable::commandForceRebuild, this, _1, _2, _3)))
            : Command::CommandHandler(std::tr1::bind(&BaseProxCommandable::commandForceRebuild, this, _1, _2, _3))
        );
    }
}

} // namespace Pinto
} // namespace Sirikata

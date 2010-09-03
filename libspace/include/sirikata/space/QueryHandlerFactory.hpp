/*  Sirikata
 *  QueryHandlerFactory.cpp
 *
 *  Copyright (c) 2010, Ewen Cheslack-Postava
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

#include <sirikata/core/util/Platform.hpp>
#include <sirikata/core/options/Options.hpp>

#include <prox/BruteForceQueryHandler.hpp>
#include <prox/RTreeQueryHandler.hpp>
#include <prox/RTreeCutQueryHandler.hpp>

namespace Sirikata {

/** Creates a Prox::QueryHandler of the specified type. Parses the arguments
 *  specified and passes them to the query handler constructor.
 */
template<typename SimulationTraits>
Prox::QueryHandler<SimulationTraits>* QueryHandlerFactory(const String& type, const String& args) {
    static OptionValue* branching = NULL;
    if (branching == NULL) {
        branching = new OptionValue("branching", "10", Sirikata::OptionValueType<uint32>(), "Number of children each node should have.");
        Sirikata::InitializeClassOptions ico("query_handler", NULL,
            branching,
            NULL);
    }

    assert(branching != NULL);

    // Since these options end up being shared if you instantiate multiple
    // QueryHandlers, reset them each time.
    branching->unsafeAs<uint32>() = 10;

    OptionSet* optionsSet = OptionSet::getOptions("query_handler", NULL);
    optionsSet->parse(args);

    if (type == "brute") {
        return new Prox::BruteForceQueryHandler<SimulationTraits>();
    }
    else if (type == "rtree") {
        return new Prox::RTreeQueryHandler<SimulationTraits>(branching->unsafeAs<uint32>());
    }
    else if (type == "rtreecut") {
        return new Prox::RTreeCutQueryHandler<SimulationTraits>(branching->unsafeAs<uint32>(), false);
    }
    else if (type == "rtreecutagg") {
        return new Prox::RTreeCutQueryHandler<SimulationTraits>(branching->unsafeAs<uint32>(), true);
    }
    else {
        return NULL;
    }
}

} // namespace Sirikata

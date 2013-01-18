// Copyright (c) 2010 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

// NOTE: This isn't really ideal since we shouldn't really depend on libprox
// within libcore. However, this works fine since it's just a header and makes
// it available to libspace plugins, liboh plugins, and pinto. DO NOT add
// anything which requires exporting symbols or an implementation file.

#include <sirikata/core/util/Platform.hpp>
#include <sirikata/core/options/Options.hpp>

#include <prox/geom/BruteForceQueryHandler.hpp>
#include <prox/geom/RTreeAngleQueryHandler.hpp>
#include <prox/geom/RTreeDistanceQueryHandler.hpp>
#include <prox/geom/RTreeCutQueryHandler.hpp>
#include <prox/geom/LevelQueryHandler.hpp>
#include <prox/geom/RebuildingQueryHandler.hpp>

#include <prox/manual/RTreeManualQueryHandler.hpp>

namespace Sirikata {

// Helper for main factory function below
template<typename SimulationTraits, typename NodeDataType>
Prox::QueryHandler<SimulationTraits>* QueryHandlerFactoryWithNodeData(const String& type, const String& args, bool rebuilding = true) {
    static OptionValue* branching = NULL;
    static OptionValue* rebuild_batch_size = NULL;
    if (branching == NULL) {
        branching = new OptionValue("branching", "10", Sirikata::OptionValueType<uint32>(), "Number of children each node should have.");
        rebuild_batch_size = new OptionValue("rebuild-batch-size", "10", Sirikata::OptionValueType<uint32>(), "Number of queries to transition on each iteration when rebuilding. Keep this small to avoid long latencies between updates.");
        Sirikata::InitializeClassOptions ico("query_handler", NULL,
            branching,
            rebuild_batch_size,
            NULL);
    }

    assert(branching != NULL);

    // Since these options end up being shared if you instantiate multiple
    // QueryHandlers, reset them each time.
    branching->unsafeAs<uint32>() = 10;

    OptionSet* optionsSet = OptionSet::getOptions("query_handler", NULL);
    optionsSet->parse(args);

    if (type == "brute") {
        if (rebuilding)
            return new Prox::RebuildingQueryHandler<SimulationTraits>(
                Prox::BruteForceQueryHandler<SimulationTraits>::Constructor(), rebuild_batch_size->unsafeAs<uint32>()
            );
        else
            return new Prox::BruteForceQueryHandler<SimulationTraits>();

    }
    else if (type == "rtree") {
        if (rebuilding)
            return new Prox::RebuildingQueryHandler<SimulationTraits>(
                Prox::RTreeAngleQueryHandler<SimulationTraits, NodeDataType>::Constructor(branching->unsafeAs<uint32>()), rebuild_batch_size->unsafeAs<uint32>()
            );
        else
            return new Prox::RTreeAngleQueryHandler<SimulationTraits, NodeDataType>(branching->unsafeAs<uint32>());
    }
    else if (type == "rtreedist" || type == "dist") {
        if (rebuilding)
            return new Prox::RebuildingQueryHandler<SimulationTraits>(
                Prox::RTreeDistanceQueryHandler<SimulationTraits, NodeDataType>::Constructor(branching->unsafeAs<uint32>()), rebuild_batch_size->unsafeAs<uint32>()
            );
        else
            return new Prox::RTreeDistanceQueryHandler<SimulationTraits, NodeDataType>(branching->unsafeAs<uint32>());
    }
    else if (type == "rtreecut") {
        if (rebuilding)
            return new Prox::RebuildingQueryHandler<SimulationTraits>(
                Prox::RTreeCutQueryHandler<SimulationTraits, NodeDataType>::Constructor(branching->unsafeAs<uint32>(), false), rebuild_batch_size->unsafeAs<uint32>()
            );
        else
            return new Prox::RTreeCutQueryHandler<SimulationTraits, NodeDataType>(branching->unsafeAs<uint32>(), false);
    }
    else if (type == "rtreecutagg") {
        if (rebuilding)
            return new Prox::RebuildingQueryHandler<SimulationTraits>(
                Prox::RTreeCutQueryHandler<SimulationTraits, NodeDataType>::Constructor(branching->unsafeAs<uint32>(), true), rebuild_batch_size->unsafeAs<uint32>()
            );
        else
            return new Prox::RTreeCutQueryHandler<SimulationTraits, NodeDataType>(branching->unsafeAs<uint32>(), true);
    }
    else if (type == "level") {
        if (rebuilding)
            return new Prox::RebuildingQueryHandler<SimulationTraits>(
                Prox::LevelQueryHandler<SimulationTraits, NodeDataType>::Constructor(branching->unsafeAs<uint32>()), rebuild_batch_size->unsafeAs<uint32>()
            );
        else
            return new Prox::LevelQueryHandler<SimulationTraits, NodeDataType>(branching->unsafeAs<uint32>());
    }
    else {
        return NULL;
    }
}

/** Creates a Prox::QueryHandler of the specified type. Parses the arguments
 *  specified and passes them to the query handler constructor.
 */
template<typename SimulationTraits>
Prox::QueryHandler<SimulationTraits>* QueryHandlerFactory(const String& type, const String& args, const String& per_node_data, bool rebuilding = true) {
    if (per_node_data == "bounds")
        return QueryHandlerFactoryWithNodeData<SimulationTraits, Prox::BoundingSphereData<SimulationTraits> >(type, args, rebuilding);
    else if (per_node_data == "maxsize")
        return QueryHandlerFactoryWithNodeData<SimulationTraits, Prox::MaxSphereData<SimulationTraits> >(type, args, rebuilding);
    else if (per_node_data == "similarmaxsize")
        return QueryHandlerFactoryWithNodeData<SimulationTraits, Prox::SimilarMaxSphereData<SimulationTraits> >(type, args, rebuilding);
    else
        return NULL;
}







// Helper for real factory function below
template<typename SimulationTraits, typename NodeDataType>
Prox::ManualQueryHandler<SimulationTraits>* ManualQueryHandlerFactoryWithNodeData(const String& type, const String& args, bool rebuilding = true) {
    static OptionValue* branching = NULL;
    static OptionValue* rebuild_batch_size = NULL;
    if (branching == NULL) {
        branching = new OptionValue("branching", "10", Sirikata::OptionValueType<uint32>(), "Number of children each node should have.");
        rebuild_batch_size = new OptionValue("rebuild-batch-size", "10", Sirikata::OptionValueType<uint32>(), "Number of queries to transition on each iteration when rebuilding. Keep this small to avoid long latencies between updates.");
        Sirikata::InitializeClassOptions ico("manual_query_handler", NULL,
            branching,
            rebuild_batch_size,
            NULL);
    }

    assert(branching != NULL);

    // Since these options end up being shared if you instantiate multiple
    // QueryHandlers, reset them each time.
    branching->unsafeAs<uint32>() = 10;

    OptionSet* optionsSet = OptionSet::getOptions("manual_query_handler", NULL);
    optionsSet->parse(args);

    if (type == "rtreecut") {
        return new Prox::RTreeManualQueryHandler<SimulationTraits, NodeDataType>(branching->unsafeAs<uint32>());
    }
    else {
        return NULL;
    }
}

/** Creates a Prox::QueryHandler of the specified type. Parses the arguments
 *  specified and passes them to the query handler constructor.
 */
template<typename SimulationTraits>
Prox::ManualQueryHandler<SimulationTraits>* ManualQueryHandlerFactory(const String& type, const String& args, const String& per_node_data, bool rebuilding = true) {
    if (per_node_data == "bounds")
        return ManualQueryHandlerFactoryWithNodeData<SimulationTraits, Prox::BoundingSphereData<SimulationTraits> >(type, args, rebuilding);
    else if (per_node_data == "maxsize")
        return ManualQueryHandlerFactoryWithNodeData<SimulationTraits, Prox::MaxSphereData<SimulationTraits> >(type, args, rebuilding);
    else if (per_node_data == "similarmaxsize")
        return ManualQueryHandlerFactoryWithNodeData<SimulationTraits, Prox::SimilarMaxSphereData<SimulationTraits> >(type, args, rebuilding);
    else
        return NULL;
}

} // namespace Sirikata

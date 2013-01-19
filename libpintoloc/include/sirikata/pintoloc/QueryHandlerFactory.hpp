// Copyright (c) 2010 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_PINTOLOC_QUERY_HANDLER_FACTORY_HPP_
#define _SIRIKATA_PINTOLOC_QUERY_HANDLER_FACTORY_HPP_

#include <sirikata/core/util/Platform.hpp>
#include <sirikata/core/util/Factory.hpp>
#include <sirikata/core/util/Singleton.hpp>

#include <sirikata/core/options/Options.hpp>

#include <prox/geom/BruteForceQueryHandler.hpp>
#include <prox/geom/RTreeAngleQueryHandler.hpp>
#include <prox/geom/RTreeDistanceQueryHandler.hpp>
#include <prox/geom/RTreeCutQueryHandler.hpp>
#include <prox/geom/LevelQueryHandler.hpp>
#include <prox/geom/RebuildingQueryHandler.hpp>

#include <prox/manual/RTreeManualQueryHandler.hpp>

namespace Sirikata {

// Implementation note: it would be nice to just use a templated singleton
// factory class, but because of the different libraries and binaries this would
// be instantiated from, it seems to make it impossible to get the
// exports/imports of symbols working properly. Therefore, we just provide a
// templated factory class which users of this code are responsible for making
// sure are singletons.

template<typename SimulationTraits, typename QueryHandlerType, typename RealQueryHandlerFactoryType>
class GenericQueryHandlerFactory :
        public Factory2<QueryHandlerType*, const String& /*args*/, bool /*rebuilding*/>
{
    typedef Factory2<QueryHandlerType*, const String& /*args*/, bool /*rebuilding*/> FactoryType;
    typedef typename FactoryType::ConstructorType ConstructorType;
public:
    typedef QueryHandlerType QueryHandler;

    bool unregisterConstructor(const String& handler_type, const String& per_node_data) {
        return FactoryType::unregisterConstructor(constructorName(handler_type, per_node_data));
    }

    bool registerConstructor(const String& handler_type, const String& per_node_data,
                             const ConstructorType &constructor,
                             bool isDefault=false) {
        return FactoryType::registerConstructor(
            constructorName(handler_type, per_node_data),
            constructor, isDefault
        );
    }

    bool hasConstructor(const String& handler_type, const String& per_node_data) const {
        const_cast<RealQueryHandlerFactoryType*>(
            static_cast<const RealQueryHandlerFactoryType*>(this)
        )->registerBuiltInConstructors(); // Makes sure built-in's are already there
        return FactoryType::hasConstructor(constructorName(handler_type, per_node_data));
    }

    const ConstructorType& getConstructorOrDefault(const String& handler_type, const String& per_node_data) const {
        const_cast<RealQueryHandlerFactoryType*>(
            static_cast<const RealQueryHandlerFactoryType*>(this)
        )->registerBuiltInConstructors(); // Makes sure built-in's are already there
        return FactoryType::getConstructorOrDefault(constructorName(handler_type, per_node_data));
    }

    const ConstructorType& getConstructor(const String& handler_type, const String& per_node_data) const {
        const_cast<RealQueryHandlerFactoryType*>(
            static_cast<const RealQueryHandlerFactoryType*>(this)
        )->registerBuiltInConstructors(); // Makes sure built-in's are already there
        return FactoryType::getConstructor(constructorName(handler_type, per_node_data));
    }

private:
    // To map to a normal factory, we need a single string to describe the
    // constructor. This is our standard way of combining them, hopefully
    // avoiding conflicts with any real names.
    String constructorName(const String& handler_type, const String& per_node_data) const {
        return handler_type + "__" + per_node_data;
    }
}; // class GenericQueryHandlerFactory



template<typename SimulationTraits>
class GeomQueryHandlerFactory : public GenericQueryHandlerFactory<SimulationTraits, Prox::QueryHandler<SimulationTraits>, GeomQueryHandlerFactory<SimulationTraits> >
{
public:
    typedef typename GeomQueryHandlerFactory<SimulationTraits>::QueryHandler QueryHandler;

    /** Registers the built-in set of constructors. This is safe to call
     *  repeatedly, so an easy way to ensure the built-in's are already
     *  available is to call this before calling getConstructor(). This may be
     *  simpler and cleaner than finding a place to call it once globally for
     *  the entire binary. Therefore, we just call it automatically before we
     *  actually do any constructor lookups.
     */
    void registerBuiltInConstructors() {
        std::vector<String> per_node_datas;
        per_node_datas.push_back("bounds");
        per_node_datas.push_back("maxsize");
        per_node_datas.push_back("similarmaxsize");

        std::vector<String> handler_types;
        handler_types.push_back("brute");
        handler_types.push_back("rtree");
        handler_types.push_back("rtreedist");
        handler_types.push_back("dist");
        handler_types.push_back("rtreecut");
        handler_types.push_back("rtreecutagg");
        handler_types.push_back("level");

        for(uint32 handler_type = 0; handler_type < handler_types.size(); handler_type++) {
            for(uint32 per_node_data = 0; per_node_data < per_node_datas.size(); per_node_data++) {
                registerConstructor(
                    handler_types[handler_type], per_node_datas[per_node_data],
                    std::tr1::bind(&GeomQueryHandlerFactory::Construct,
                        handler_types[handler_type], per_node_datas[per_node_data], _1, _2
                    ),
                    false
                );
            }
        }
    }

private:

    // Helper for main factory function below
    template<typename NodeDataType>
    static QueryHandler* ConstructWithNodeData(const String& type, const String& args, bool rebuilding = true) {
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
    static QueryHandler* Construct(const String& type, const String& per_node_data, const String& args, bool rebuilding = true) {
        if (per_node_data == "bounds")
            return ConstructWithNodeData<Prox::BoundingSphereData<SimulationTraits> >(type, args, rebuilding);
        else if (per_node_data == "maxsize")
            return ConstructWithNodeData<Prox::MaxSphereData<SimulationTraits> >(type, args, rebuilding);
        else if (per_node_data == "similarmaxsize")
            return ConstructWithNodeData<Prox::SimilarMaxSphereData<SimulationTraits> >(type, args, rebuilding);
        else
            return NULL;
    }

}; // class GeomQueryHandlerFactory





template<typename SimulationTraits>
class ManualQueryHandlerFactory : public GenericQueryHandlerFactory<SimulationTraits, Prox::ManualQueryHandler<SimulationTraits>, ManualQueryHandlerFactory<SimulationTraits> >
{
public:
    typedef typename ManualQueryHandlerFactory<SimulationTraits>::QueryHandler QueryHandler;

    /** Registers the built-in set of constructors. This is safe to call
     *  repeatedly, so an easy way to ensure the built-in's are already
     *  available is to call this before calling getConstructor(). This may be
     *  simpler and cleaner than finding a place to call it once globally for
     *  the entire binary. Therefore, we just call it automatically before we
     *  actually do any constructor lookups.
     */
    void registerBuiltInConstructors() {
        std::vector<String> per_node_datas;
        per_node_datas.push_back("bounds");
        per_node_datas.push_back("maxsize");
        per_node_datas.push_back("similarmaxsize");

        std::vector<String> handler_types;
        handler_types.push_back("rtreecut");

        for(uint32 handler_type = 0; handler_type < handler_types.size(); handler_type++) {
            for(uint32 per_node_data = 0; per_node_data < per_node_datas.size(); per_node_data++) {
                registerConstructor(
                    handler_types[handler_type], per_node_datas[per_node_data],
                    std::tr1::bind(&ManualQueryHandlerFactory::Construct,
                        handler_types[handler_type], per_node_datas[per_node_data], _1, _2
                    ),
                    false
                );
            }
        }
    }

private:

    // Helper for real factory function below
    template<typename NodeDataType>
    static QueryHandler* ConstructWithNodeData(const String& type, const String& args, bool rebuilding = true) {
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
    static QueryHandler* Construct(const String& type, const String& per_node_data, const String& args, bool rebuilding = true) {
        if (per_node_data == "bounds")
            return ConstructWithNodeData<Prox::BoundingSphereData<SimulationTraits> >(type, args, rebuilding);
        else if (per_node_data == "maxsize")
            return ConstructWithNodeData<Prox::MaxSphereData<SimulationTraits> >(type, args, rebuilding);
        else if (per_node_data == "similarmaxsize")
            return ConstructWithNodeData<Prox::SimilarMaxSphereData<SimulationTraits> >(type, args, rebuilding);
        else
            return NULL;
    }

}; // class ManualQueryHandlerFactory

} // namespace Sirikata


#endif //_SIRIKATA_PINTOLOC_QUERY_HANDLER_FACTORY_HPP_

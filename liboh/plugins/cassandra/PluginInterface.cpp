/*  Sirikata
 *  PluginInterface.cpp
 *
 *  Copyright (c) 2011, Stanford University
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

#include <sirikata/oh/Platform.hpp>
#include <sirikata/core/options/Options.hpp>
#include "CassandraStorage.hpp"
#include "CassandraPersistedObjectSet.hpp"
#include "CassandraObjectFactory.hpp"

static int cassandraoh_plugin_refcount = 0;

namespace Sirikata {

static void InitPluginOptions() {
    Sirikata::InitializeClassOptions ico("cassandrastorage",NULL,
        new Sirikata::OptionValue("host", "localhost", Sirikata::OptionValueType<String>(), "Host name of Cassandra server"),
        new Sirikata::OptionValue("port", "9160", Sirikata::OptionValueType<int32>(), "Port number"),
        new Sirikata::OptionValue("lease-duration", "30s", Sirikata::OptionValueType<Duration>(), "Duration to register leases for. Longer times require less overhead, but also mean longer delays if an object or object host dies without cleaning up."),
        NULL);

    Sirikata::InitializeClassOptions icop("cassandrapersistedset",NULL,
        new Sirikata::OptionValue("host", "localhost", Sirikata::OptionValueType<String>(), "Host name of Cassandra server"),
        new Sirikata::OptionValue("port", "9160", Sirikata::OptionValueType<int32>(), "Port number"),
        new Sirikata::OptionValue("ohid", "0", Sirikata::OptionValueType<String>(), "Object Host ID"),
        NULL);

    Sirikata::InitializeClassOptions icof("cassandrafactory",NULL,
        new Sirikata::OptionValue("host", "localhost", Sirikata::OptionValueType<String>(), "Host name of Cassandra server"),
        new Sirikata::OptionValue("port", "9160", Sirikata::OptionValueType<int32>(), "Port number"),
        new Sirikata::OptionValue("ohid", "default", Sirikata::OptionValueType<String>(), "Object Host ID"),
        NULL);
}

static OH::Storage* createCassandraStorage(ObjectHostContext* ctx, const String& args) {
    OptionSet* optionsSet = OptionSet::getOptions("cassandrastorage",NULL);
    optionsSet->parse(args);

    String host = optionsSet->referenceOption("host")->as<String>();
    int32 port = optionsSet->referenceOption("port")->as<int32>();
    Duration lease_duration = optionsSet->referenceOption("lease-duration")->as<Duration>();

    return new OH::CassandraStorage(ctx, host, port, lease_duration);
}

static OH::PersistedObjectSet* createCassandraPersistedObjectSet(ObjectHostContext* ctx, const String& args) {
    OptionSet* optionsSet = OptionSet::getOptions("cassandrapersistedset",NULL);
    optionsSet->parse(args);

    String host = optionsSet->referenceOption("host")->as<String>();
    int32 port = optionsSet->referenceOption("port")->as<int32>();
    String ohid = optionsSet->referenceOption("ohid")->as<String>();

    return new OH::CassandraPersistedObjectSet(ctx, host, port, ohid);
}

static ObjectFactory* createCassandraObjectFactory(ObjectHostContext* ctx, ObjectHost* oh, const SpaceID& space, const String& args) {
    OptionSet* optionsSet = OptionSet::getOptions("cassandrafactory",NULL);
    optionsSet->parse(args);

    String host = optionsSet->referenceOption("host")->as<String>();
    int32 port = optionsSet->referenceOption("port")->as<int32>();
    String ohid = optionsSet->referenceOption("ohid")->as<String>();

    return new CassandraObjectFactory(ctx, oh, space, host, port, ohid);
}

} // namespace Sirikata

SIRIKATA_PLUGIN_EXPORT_C void init() {
    using namespace Sirikata;
    if (cassandraoh_plugin_refcount==0) {
        InitPluginOptions();
        using std::tr1::placeholders::_1;
        OH::StorageFactory::getSingleton()
            .registerConstructor("cassandra",
                                 std::tr1::bind(&createCassandraStorage, std::tr1::placeholders::_1, std::tr1::placeholders::_2));
        OH::PersistedObjectSetFactory::getSingleton()
            .registerConstructor("cassandra",
                                 std::tr1::bind(&createCassandraPersistedObjectSet, std::tr1::placeholders::_1, std::tr1::placeholders::_2));
        ObjectFactoryFactory::getSingleton()
            .registerConstructor("cassandra",
                                 std::tr1::bind(&createCassandraObjectFactory, std::tr1::placeholders::_1, std::tr1::placeholders::_2, std::tr1::placeholders::_3, std::tr1::placeholders::_4));
    }
    cassandraoh_plugin_refcount++;
}

SIRIKATA_PLUGIN_EXPORT_C int increfcount() {
    return ++cassandraoh_plugin_refcount;
}
SIRIKATA_PLUGIN_EXPORT_C int decrefcount() {
    assert(cassandraoh_plugin_refcount>0);
    return --cassandraoh_plugin_refcount;
}

SIRIKATA_PLUGIN_EXPORT_C void destroy() {
    using namespace Sirikata;
    if (cassandraoh_plugin_refcount==0) {
        OH::StorageFactory::getSingleton().unregisterConstructor("cassandra");
        OH::PersistedObjectSetFactory::getSingleton().unregisterConstructor("cassandra");
        ObjectFactoryFactory::getSingleton().unregisterConstructor("cassandra");
    }
}

SIRIKATA_PLUGIN_EXPORT_C const char* name() {
    return "oh-cassandra";
}

SIRIKATA_PLUGIN_EXPORT_C int refcount() {
    return cassandraoh_plugin_refcount;
}

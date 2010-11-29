/*  Sirikata
 *  Options.cpp
 *
 *  Copyright (c) 2009, Ewen Cheslack-Postava
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

#include "Options.hpp"
#include <sirikata/core/options/Options.hpp>
#include <sirikata/core/util/Time.hpp>

namespace Sirikata {

void InitSpaceOptions() {
    InitializeClassOptions::module(SIRIKATA_OPTIONS_MODULE)

        .addOption(new OptionValue(OPT_CONFIG_FILE,"",Sirikata::OptionValueType<String>(),"Configuration file to load."))

        .addOption(new OptionValue(OPT_SPACE_PLUGINS,"weight-exp,weight-sqr,weight-const,space-local,space-standard,colladamodels",Sirikata::OptionValueType<String>(),"Plugin list to load."))

        .addOption(new OptionValue("spacestreamlib","tcpsst",Sirikata::OptionValueType<String>(),"Which library to use to communicate with the object host"))
        .addOption(new OptionValue("spacestreamoptions","--send-buffer-size=32768 --parallel-sockets=1 --no-delay=true",Sirikata::OptionValueType<String>(),"TCPSST stream options such as how many bytes to collect for sending during an ongoing asynchronous send call."))

        .addOption(new OptionValue("id", "1", Sirikata::OptionValueType<ServerID>(), "Server ID for this server"))

        .addOption(new OptionValue("capexcessbandwidth", "false", Sirikata::OptionValueType<bool>(), "Total bandwidth for this server in bytes per second"))

        .addOption(new OptionValue(SERVER_QUEUE, "fair", Sirikata::OptionValueType<String>(), "The type of ServerMessageQueue to use for routing."))
        .addOption(new OptionValue(SERVER_QUEUE_LENGTH, "8192", Sirikata::OptionValueType<uint32>(), "Length of queue for each server."))
        .addOption(new OptionValue(SERVER_RECEIVER, "fair", Sirikata::OptionValueType<String>(), "The type of ServerMessageReceiver to use for routing."))
        .addOption(new OptionValue(SERVER_ODP_FLOW_SCHEDULER, "region", Sirikata::OptionValueType<String>(), "The type of ODPFlowScheduler to use for routing."))
        .addOption(new OptionValue(FORWARDER_RECEIVE_QUEUE_SIZE, "16384", Sirikata::OptionValueType<uint32>(), "The type of ODPFlowScheduler to use for routing."))
        .addOption(new OptionValue(FORWARDER_SEND_QUEUE_SIZE, "65536", Sirikata::OptionValueType<uint32>(), "The type of ODPFlowScheduler to use for routing."))

        .addOption(new OptionValue(NETWORK_TYPE, "tcp", Sirikata::OptionValueType<String>(), "The networking subsystem to use."))

        .addOption(new OptionValue(OSEG,"local",Sirikata::OptionValueType<String>(),"Specifies which type of oseg to use."))
        .addOption(new OptionValue(OSEG_OPTIONS,"",Sirikata::OptionValueType<String>(),"Specifies arguments to OSeg."))

        .addOption(new OptionValue(OSEG_LOOKUP_QUEUE_SIZE, "2000", Sirikata::OptionValueType<uint32>(), "Number of new lookups you can have on oseg lookup queue."))

        .addOption(new OptionValue(OSEG_CACHE_SIZE, "200", Sirikata::OptionValueType<uint32>(), "Maximum number of entries in the OSeg cache."))

        .addOption(new OptionValue(CACHE_SELECTOR,CACHE_TYPE_ORIGINAL_LRU,Sirikata::OptionValueType<String>(),"Which caching algorithm to use."))

         .addOption(new OptionValue(CACHE_COMM_SCALING,"1.0",Sirikata::OptionValueType<double>(),"What the communication falloff function scaling factor is."))
         .addOption(new OptionValue("send-capacity-overestimate","80000",Sirikata::OptionValueType<double>(),"How much to overestimate send capacity when queue is not blocked."))
         .addOption(new OptionValue("receive-capacity-overestimate","1",Sirikata::OptionValueType<double>(),"How much to overestimate recv capacity when queue is not blocked."))
        .addOption(new OptionValue(OSEG_CACHE_CLEAN_GROUP_SIZE, "25", Sirikata::OptionValueType<uint32>(), "Number of items to remove from the OSeg cache when it reaches the maximum size."))
        .addOption(new OptionValue(OSEG_CACHE_ENTRY_LIFETIME, "8s", Sirikata::OptionValueType<Duration>(), "Maximum lifetime for an OSeg cache entry."))

        .addOption(new OptionValue(CSEG, "uniform", Sirikata::OptionValueType<String>(), "Type of Coordinate Segmentation implementation to use."))
        .addOption(new OptionValue("cseg-service-host", "meru00", Sirikata::OptionValueType<String>(), "Hostname of machine running the CSEG service (running with --cseg=distributed)"))
        .addOption(new OptionValue("cseg-service-tcp-port", "2234", Sirikata::OptionValueType<String>(), "TCP listening port number on host running the CSEG service (running with --cseg=distributed)"))

        .addOption(new OptionValue(LOC, "standard", Sirikata::OptionValueType<String>(), "Type of location service to run."))
        .addOption(new OptionValue(LOC_OPTIONS, "", Sirikata::OptionValueType<String>(), "Options to pass to Loc constructor."))
        .addOption(new OptionValue(LOC_UPDATE, "always", Sirikata::OptionValueType<String>(), "Type of location service to run."))
        .addOption(new OptionValue(LOC_UPDATE_OPTIONS, "", Sirikata::OptionValueType<String>(), "Options to pass to Loc constructor."))

        .addOption(new OptionValue(PROX_MAX_PER_RESULT, "5", Sirikata::OptionValueType<uint32>(), "Maximum number of changes to report in each result message."))

        .addOption(new OptionValue(OPT_PROX_SPLIT_DYNAMIC, "false", Sirikata::OptionValueType<bool>(), "If true, separate query handlers will be used for static and dynamic objects."))

        .addOption(new OptionValue(OPT_PROX_QUERY_RANGE, "100", Sirikata::OptionValueType<float32>(), "The range of queries when using range queries instead of solid angle queries."))

        .addOption(new OptionValue(OPT_PROX_SERVER_QUERY_HANDLER_TYPE, "rtreecut", Sirikata::OptionValueType<String>(), "Type of libprox query handler to use for queries from servers."))
        .addOption(new OptionValue(OPT_PROX_SERVER_QUERY_HANDLER_OPTIONS, "", Sirikata::OptionValueType<String>(), "Options for the query handler."))

        .addOption(new OptionValue(OPT_PROX_OBJECT_QUERY_HANDLER_TYPE, "rtreecut", Sirikata::OptionValueType<String>(), "Type of libprox query handler to use for queries from servers."))
        .addOption(new OptionValue(OPT_PROX_OBJECT_QUERY_HANDLER_OPTIONS, "", Sirikata::OptionValueType<String>(), "Options for the query handler."))

        .addOption(new OptionValue(OPT_PINTO,"local",Sirikata::OptionValueType<String>(),"Specifies which type of Pinto to use."))
        .addOption(new OptionValue(OPT_PINTO_OPTIONS,"",Sirikata::OptionValueType<String>(),"Specifies arguments to Pinto."))

      .addOption(new OptionValue("route-object-message-buffer", "64", Sirikata::OptionValueType<size_t>(), "size of the buffer between network and main strand for space server message routing"))

      ;
}

} // namespace Sirikata

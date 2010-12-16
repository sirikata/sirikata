/*  Sirikata
 *  main.cpp
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

#include <sirikata/core/util/Platform.hpp>
#include <sirikata/core/network/IOServiceFactory.hpp>
#include <sirikata/core/network/IOService.hpp>
#include <sirikata/core/network/IOStrand.hpp>

#include <sirikata/core/util/Timer.hpp>

#include "Options.hpp"
#include <sirikata/core/options/CommonOptions.hpp>
#include <sirikata/core/util/PluginManager.hpp>
#include <sirikata/core/trace/Trace.hpp>
#include "Analysis.hpp"
#include "MessageLatency.hpp"
#include "ObjectLatency.hpp"
#include "FlowStats.hpp"
//#include "Visualization.hpp"

void *main_loop(void *);

bool is_analysis() {
    using namespace Sirikata;

    if (GetOptionValue<bool>(ANALYSIS_LOC) ||
        GetOptionValue<String>(ANALYSIS_LOCVIS) != "none" ||
        GetOptionValue<bool>(ANALYSIS_LATENCY) ||
        GetOptionValue<bool>(ANALYSIS_OBJECT_LATENCY) ||
        GetOptionValue<bool>(ANALYSIS_MESSAGE_LATENCY) ||
        GetOptionValue<bool>(ANALYSIS_BANDWIDTH) ||
        !GetOptionValue<String>(ANALYSIS_WINDOWED_BANDWIDTH).empty() ||
        GetOptionValue<bool>(ANALYSIS_OSEG) ||
        GetOptionValue<bool>(ANALYSIS_OBJECT_LATENCY) ||
        GetOptionValue<bool>(ANALYSIS_LOC_LATENCY) ||
        !GetOptionValue<String>(ANALYSIS_PROX_DUMP).empty() ||
        GetOptionValue<bool>(ANALYSIS_FLOW_STATS))
        return true;

    return false;
}

int main(int argc, char** argv) {
    using namespace Sirikata;

    DynamicLibrary::Initialize();

    InitOptions();
    Trace::Trace::InitOptions();
    InitAnalysisOptions();
    ParseOptions(argc, argv);

    PluginManager plugins;
    plugins.loadList( GetOptionValue<String>(OPT_PLUGINS) );
    plugins.loadList( GetOptionValue<String>(OPT_ANALYSIS_PLUGINS) );

    assert(is_analysis());

    String trace_file = "analysis.trace";

    // Compute the starting date/time
    String start_time_str = GetOptionValue<String>("wait-until");
    Time start_time = start_time_str.empty() ? Timer::now() : Timer::getSpecifiedDate( start_time_str );
    start_time += GetOptionValue<Duration>("wait-additional");

    Duration duration = GetOptionValue<Duration>("duration");

    Network::IOService* ios = Network::IOServiceFactory::makeIOService();
    Network::IOStrand* mainStrand = ios->createStrand();


    BoundingBox3f region = GetOptionValue<BoundingBox3f>("region");
    Vector3ui32 layout = GetOptionValue<Vector3ui32>("layout");


    uint32 max_space_servers = GetOptionValue<uint32>("max-servers");
    if (max_space_servers == 0)
      max_space_servers = layout.x * layout.y * layout.z;
    //uint32 num_oh_servers = GetOptionValue<uint32>("num-oh");
    //uint32 nservers = max_space_servers + num_oh_servers;
    uint32 nservers = GetOptionValue<uint32>(ANALYSIS_TOTAL_NUM_ALL_SERVERS);

    srand( GetOptionValue<uint32>("rand-seed") );

    if ( GetOptionValue<bool>(ANALYSIS_LOC) ) {
        LocationErrorAnalysis lea(STATS_TRACE_FILE, nservers);
        printf("Total error: %f\n", (float)lea.globalAverageError( Duration::milliseconds((int64)10)));
        exit(0);
    }
    else if ( GetOptionValue<String>(ANALYSIS_LOCVIS) != "none") {
        assert(false);
    }
    else if ( GetOptionValue<bool>(ANALYSIS_LATENCY) ) {
        LatencyAnalysis la(STATS_TRACE_FILE,nservers);

        exit(0);
    }
    else if ( GetOptionValue<bool>(ANALYSIS_OBJECT_LATENCY) ) {
        ObjectLatencyAnalysis la(STATS_TRACE_FILE,nservers);
        std::ofstream histogram_data("distance_latency_histogram.csv");
        la.printHistogramDistanceData(histogram_data,10);
        la.printTotalAverage(histogram_data);
        histogram_data.close();
        exit(0);
    }
    else if ( GetOptionValue<bool>(ANALYSIS_MESSAGE_LATENCY) ) {
        uint16 ping_port=14050;//OBJECT_PORT_PING;
        uint32 unservers=nservers;
        MessageLatencyFilters filter(&ping_port,&unservers,//filter by created @ object host
                       &unservers);//filter by destroyed @ object host
        MessageLatencyFilters nilfilter;
        MessageLatencyFilters pingfilter(&ping_port);
        MessageLatencyAnalysis(STATS_TRACE_FILE,nservers,pingfilter/*,"stage_samples.txt"*/);
        exit(0);
    }
    else if ( GetOptionValue<bool>(ANALYSIS_BANDWIDTH) ) {
        BandwidthAnalysis ba(STATS_TRACE_FILE, max_space_servers);
        printf("Send rates\n");
        for(ServerID sender = 1; sender <= max_space_servers; sender++) {
            for(ServerID receiver = 1; receiver <= max_space_servers; receiver++) {
                ba.computeSendRate(sender, receiver);
            }
        }
        printf("Receive rates\n");
        for(ServerID sender = 1; sender <= max_space_servers; sender++) {
            for(ServerID receiver = 1; receiver <= max_space_servers; receiver++) {
                ba.computeReceiveRate(sender, receiver);
            }
        }

        ba.computeJFI(1);
        exit(0);
    }
    else if ( !GetOptionValue<String>(ANALYSIS_WINDOWED_BANDWIDTH).empty() ) {
        String windowed_analysis_type = GetOptionValue<String>(ANALYSIS_WINDOWED_BANDWIDTH);

        String windowed_analysis_send_filename = "windowed_bandwidth_";
        windowed_analysis_send_filename += windowed_analysis_type;
        windowed_analysis_send_filename += "_send.dat";
        String windowed_analysis_receive_filename = "windowed_bandwidth_";
        windowed_analysis_receive_filename += windowed_analysis_type;
        windowed_analysis_receive_filename += "_receive.dat";

        String queue_info_filename = "queue_info_";
        queue_info_filename += windowed_analysis_type;
        queue_info_filename += ".dat";


        String windowed_queue_info_send_filename = "windowed_queue_info_send_";
        windowed_queue_info_send_filename += windowed_analysis_type;
        windowed_queue_info_send_filename += ".dat";
        String windowed_queue_info_receive_filename = "windowed_queue_info_receive_";
        windowed_queue_info_receive_filename += windowed_analysis_type;
        windowed_queue_info_receive_filename += ".dat";

        std::ofstream windowed_analysis_send_file(windowed_analysis_send_filename.c_str());
        std::ofstream windowed_analysis_receive_file(windowed_analysis_receive_filename.c_str());

        std::ofstream queue_info_file(queue_info_filename.c_str());

        std::ofstream windowed_queue_info_send_file(windowed_queue_info_send_filename.c_str());
        std::ofstream windowed_queue_info_receive_file(windowed_queue_info_receive_filename.c_str());

        Duration window = GetOptionValue<Duration>(ANALYSIS_WINDOWED_BANDWIDTH_WINDOW);
        Duration sample_rate = GetOptionValue<Duration>(ANALYSIS_WINDOWED_BANDWIDTH_RATE);
        BandwidthAnalysis ba(STATS_TRACE_FILE, max_space_servers);
        Time start_time = Time::null();
        Time end_time = start_time + duration;
        printf("Send rates\n");
        for(ServerID sender = 1; sender <= max_space_servers; sender++) {
            for(ServerID receiver = 1; receiver <= max_space_servers; receiver++) {
                if (windowed_analysis_type == "datagram")
                    ba.computeWindowedDatagramSendRate(sender, receiver, window, sample_rate, start_time, end_time, std::cout, windowed_analysis_send_file);
            }
        }
        printf("Receive rates\n");
        for(ServerID sender = 1; sender <= max_space_servers; sender++) {
            for(ServerID receiver = 1; receiver <= max_space_servers; receiver++) {
                if (windowed_analysis_type == "datagram")
                    ba.computeWindowedDatagramReceiveRate(sender, receiver, window, sample_rate, start_time, end_time, std::cout, windowed_analysis_receive_file);
            }
        }

        exit(0);
    }
    else if ( GetOptionValue<bool>(ANALYSIS_OSEG) )
    {
      //bftm additional object messages log file creation.
        int osegProcessedAfterSeconds = GetOptionValue<int>(OSEG_ANALYZE_AFTER);


        //oseg migrates
        String object_segmentation_filename = "oseg_object_segmentation_file";
        object_segmentation_filename += ".dat";

        ObjectSegmentationAnalysis osegAnalysis (STATS_TRACE_FILE, max_space_servers);
        std::ofstream object_seg_stream (object_segmentation_filename.c_str());
        osegAnalysis.printData(object_seg_stream);
        object_seg_stream.flush();
        object_seg_stream.close();

        //oseg craq lookups
        String object_segmentation_craq_lookup_filename = "oseg_object_segmentation_craq_lookup_file";
        object_segmentation_craq_lookup_filename += ".dat";

        ObjectSegmentationCraqLookupRequestsAnalysis craqLookupAnalysis(STATS_TRACE_FILE,max_space_servers);
        std::ofstream oseg_craq_lookup_stream(object_segmentation_craq_lookup_filename.c_str());
        craqLookupAnalysis.printData(oseg_craq_lookup_stream);
        oseg_craq_lookup_stream.flush();
        oseg_craq_lookup_stream.close();


        //oseg not on server lookups
        String object_segmentation_lookup_not_on_server_filename = "oseg_object_segmentation_lookup_not_on_server_file";
        object_segmentation_lookup_not_on_server_filename += ".dat";

        ObjectSegmentationLookupNotOnServerRequestsAnalysis lookupNotOnServerAnalysis(STATS_TRACE_FILE,max_space_servers);
        std::ofstream oseg_lookup_not_on_server_stream(object_segmentation_lookup_not_on_server_filename.c_str());
        lookupNotOnServerAnalysis.printData(oseg_lookup_not_on_server_stream);
        oseg_lookup_not_on_server_stream.flush();
        oseg_lookup_not_on_server_stream.close();


        //oseg processed lookups
        String object_segmentation_processed_filename = "oseg_object_segmentation_processed_file";
        object_segmentation_processed_filename += ".dat";


        ObjectSegmentationProcessedRequestsAnalysis processedAnalysis(STATS_TRACE_FILE,max_space_servers);
        std::ofstream oseg_process_stream(object_segmentation_processed_filename.c_str());

        processedAnalysis.printData(oseg_process_stream, true, osegProcessedAfterSeconds);
        oseg_process_stream.flush();
        oseg_process_stream.close();


        //oseg cumulative trace code
        String  object_segmentation_cumulative_filename_csv = "oseg_object_segmentation_cumulative_file";
        object_segmentation_cumulative_filename_csv += ".csv";

        OSegCumulativeTraceAnalysis cumulativeOsegAnalysis(STATS_TRACE_FILE,max_space_servers, osegProcessedAfterSeconds);

        std::ofstream oseg_cumulative_stream_csv(object_segmentation_cumulative_filename_csv.c_str());

        cumulativeOsegAnalysis.printData(oseg_cumulative_stream_csv);
        oseg_cumulative_stream_csv.flush();
        oseg_cumulative_stream_csv.close();


        //completed round trip migrate times
        String migration_round_trip_times_filename = "oseg_migration_round_trip_times_file";
        migration_round_trip_times_filename += ".dat";

        ObjectMigrationRoundTripAnalysis obj_mig_rdTripAnalysis(STATS_TRACE_FILE,max_space_servers);

        std::ofstream mig_rd_trip_times_stream(migration_round_trip_times_filename.c_str());

        obj_mig_rdTripAnalysis.printData(mig_rd_trip_times_stream, osegProcessedAfterSeconds);

        mig_rd_trip_times_stream.flush();
        mig_rd_trip_times_stream.close();

        //oseg shut down
        String oseg_shutdown_filename = "oseg_shutdown_file";
        oseg_shutdown_filename += ".dat";

        OSegShutdownAnalysis oseg_shutdown_analysis(STATS_TRACE_FILE,max_space_servers);

        std::ofstream oseg_shutdown_analysis_stream (oseg_shutdown_filename.c_str());
        oseg_shutdown_analysis.printData(oseg_shutdown_analysis_stream);

        oseg_shutdown_analysis_stream.flush();
        oseg_shutdown_analysis_stream.close();


        //oseg tracked set results analysis
        String oseg_tracked_set_results_filename = "oseg_tracked_set_results_file";
        oseg_tracked_set_results_filename += ".dat";

        OSegTrackedSetResultsAnalysis oseg_tracked_set_res_analysis(STATS_TRACE_FILE,max_space_servers);

        std::ofstream oseg_tracked_set_results_analysis_stream (oseg_tracked_set_results_filename.c_str());
        oseg_tracked_set_res_analysis.printData(oseg_tracked_set_results_analysis_stream,osegProcessedAfterSeconds);

        oseg_tracked_set_results_analysis_stream.flush();
        oseg_tracked_set_results_analysis_stream.close();


        //oseg cache response analysis
        String oseg_cache_response_filename = "oseg_cache_response_file";
        oseg_cache_response_filename += ".dat";

        OSegCacheResponseAnalysis oseg_cache_response_analysis(STATS_TRACE_FILE, max_space_servers);

        std::ofstream oseg_cached_response_analysis_stream(oseg_cache_response_filename.c_str());

        oseg_cache_response_analysis.printData(oseg_cached_response_analysis_stream,osegProcessedAfterSeconds);

        oseg_cached_response_analysis_stream.flush();
        oseg_cached_response_analysis_stream.close();

        //end cache response analysis

        //cached error analysis
        String oseg_cached_response_error_filename = "oseg_cached_response_error_file";
        oseg_cached_response_error_filename += ".dat";

        OSegCacheErrorAnalysis oseg_cached_error_response (STATS_TRACE_FILE,max_space_servers);

        std::ofstream oseg_cached_error_response_stream(oseg_cached_response_error_filename.c_str());
        oseg_cached_error_response.printData(oseg_cached_error_response_stream,osegProcessedAfterSeconds);

        oseg_cached_error_response_stream.flush();
        oseg_cached_error_response_stream.close();

        //end cached error analysis


        //end bftm additional object message log file creation.

        exit(0);
    }
    else if ( GetOptionValue<bool>(ANALYSIS_LOC_LATENCY) ) {
        LocationLatencyAnalysis(STATS_TRACE_FILE, nservers);
        exit(0);
    }
    else if ( !GetOptionValue<String>(ANALYSIS_PROX_DUMP).empty() ) {
        ProximityDumpAnalysis(STATS_TRACE_FILE, nservers, GetOptionValue<String>(ANALYSIS_PROX_DUMP));
        exit(0);
    }
    else if ( GetOptionValue<bool>(ANALYSIS_FLOW_STATS) ) {
        FlowStatsAnalysis(STATS_TRACE_FILE, nservers);
        exit(0);
    }

    delete mainStrand;
    Network::IOServiceFactory::destroyIOService(ios);

    plugins.gc();

    return 0;
}

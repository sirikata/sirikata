/*  cbr
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
 *  * Neither the name of cbr nor the names of its contributors may
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

#include "Timer.hpp"
#include "TimeSync.hpp"
#include "TimeProfiler.hpp"

#include "Options.hpp"
#include "Statistics.hpp"
#include "Analysis.hpp"
#include "Visualization.hpp"
#include "TabularServerIDMap.hpp"
#include "UniformCoordinateSegmentation.hpp"
#include "CoordinateSegmentationClient.hpp"

#include "ServerWeightCalculator.hpp"

#include "Message.hpp"

namespace {
CBR::Trace* gTrace = NULL;
CBR::SpaceContext* gSpaceContext = NULL;
}


namespace CBR {
/** Mock forwarder class, neeeded because CoordinateSegmentation uses it via SpaceContext. */
class MockForwarder : public CBR::MessageDispatcher, public CBR::MessageRouter {
public:
    MockForwarder(CBR::SpaceContext* ctx) {
        ctx->mRouter = this;
        ctx->mDispatcher = this;
    }

    virtual bool route(SERVICES svc, CBR::Message* msg, bool is_forward = false) {
        return true;
    }

    virtual bool route(CBR::Protocol::Object::ObjectMessage* msg, bool is_forward, CBR::ServerID forwardFrom = NullServerID) {
        return true;
    }
};
} // namespace CBR

void *main_loop(void *);

bool is_analysis() {
    using namespace CBR;

    if (GetOption(ANALYSIS_LOC)->as<bool>() ||
        GetOption(ANALYSIS_LOCVIS)->as<String>() != "none" ||
        GetOption(ANALYSIS_LATENCY)->as<bool>() ||
        GetOption(ANALYSIS_OBJECT_LATENCY)->as<bool>() ||
        GetOption(ANALYSIS_MESSAGE_LATENCY)->as<bool>() ||
        GetOption(ANALYSIS_BANDWIDTH)->as<bool>() ||
        !GetOption(ANALYSIS_WINDOWED_BANDWIDTH)->as<String>().empty() ||
        GetOption(ANALYSIS_OSEG)->as<bool>() ||
        GetOption(ANALYSIS_OBJECT_LATENCY)->as<bool>() ||
        GetOption(ANALYSIS_LOC_LATENCY)->as<bool>() ||
        !GetOption(ANALYSIS_PROX_DUMP)->as<String>().empty())
        return true;

    return false;
}

int main(int argc, char** argv) {
    using namespace CBR;

    InitOptions();
    ParseOptions(argc, argv);

    assert(is_analysis());

    ServerID server_id = GetOption("id")->as<ServerID>();
    String trace_file = "analysis.trace";
    gTrace = new Trace(trace_file);

    // Compute the starting date/time
    String start_time_str = GetOption("wait-until")->as<String>();
    Time start_time = start_time_str.empty() ? Timer::now() : Timer::getSpecifiedDate( start_time_str );
    start_time += GetOption("wait-additional")->as<Duration>();

    Duration duration = GetOption("duration")->as<Duration>();

    IOService* ios = IOServiceFactory::makeIOService();
    IOStrand* mainStrand = ios->createStrand();

    Time init_space_ctx_time = Time::null() + (Timer::now() - start_time);
    SpaceContext* space_context = new SpaceContext(server_id, ios, mainStrand, start_time, init_space_ctx_time, gTrace, duration);
    MockForwarder* forwarder = new MockForwarder(space_context);

    BoundingBox3f region = GetOption("region")->as<BoundingBox3f>();
    Vector3ui32 layout = GetOption("layout")->as<Vector3ui32>();


    uint32 max_space_servers = GetOption("max-servers")->as<uint32>();
    if (max_space_servers == 0)
      max_space_servers = layout.x * layout.y * layout.z;
    uint32 num_oh_servers = GetOption("num-oh")->as<uint32>();
    uint32 nservers = max_space_servers + num_oh_servers;

    srand( GetOption("rand-seed")->as<uint32>() );


    String filehandle = GetOption("serverips")->as<String>();
    std::ifstream ipConfigFileHandle(filehandle.c_str());
    ServerIDMap * server_id_map = new TabularServerIDMap(ipConfigFileHandle);
    gTrace->setServerIDMap(server_id_map);



    String cseg_type = GetOption(CSEG)->as<String>();
    CoordinateSegmentation* cseg = NULL;
    if (cseg_type == "uniform")
        cseg = new UniformCoordinateSegmentation(space_context, region, layout);
    else if (cseg_type == "client") {
      cseg = new CoordinateSegmentationClient(space_context, region, layout, server_id_map);
    }
    else {
        assert(false);
        exit(-1);
    }


    if ( GetOption(ANALYSIS_LOC)->as<bool>() ) {
        LocationErrorAnalysis lea(STATS_TRACE_FILE, nservers);
        printf("Total error: %f\n", (float)lea.globalAverageError( Duration::milliseconds((int64)10)));
        exit(0);
    }
    else if ( GetOption(ANALYSIS_LOCVIS)->as<String>() != "none") {
        String vistype = GetOption(ANALYSIS_LOCVIS)->as<String>();
        LocationVisualization lea(STATS_TRACE_FILE, nservers, space_context, cseg);

        if (vistype == "object")
            lea.displayRandomViewerError(GetOption(ANALYSIS_LOCVIS_SEED)->as<int>(), Duration::milliseconds((int64)30));
        else if (vistype == "server")
            lea.displayRandomServerError(GetOption(ANALYSIS_LOCVIS_SEED)->as<int>(), Duration::milliseconds((int64)30));

        exit(0);
    }
    else if ( GetOption(ANALYSIS_LATENCY)->as<bool>() ) {
        LatencyAnalysis la(STATS_TRACE_FILE,nservers);

        exit(0);
    }
    else if ( GetOption(ANALYSIS_OBJECT_LATENCY)->as<bool>() ) {
        ObjectLatencyAnalysis la(STATS_TRACE_FILE,nservers);
        std::ofstream histogram_data("distance_latency_histogram.csv");
        la.printHistogramDistanceData(histogram_data,10);
        histogram_data.close();
        exit(0);
    }
    else if ( GetOption(ANALYSIS_MESSAGE_LATENCY)->as<bool>() ) {
        uint16 ping_port=OBJECT_PORT_PING;
        uint32 unservers=nservers;
        MessageLatencyAnalysis::Filters filter(&ping_port,&unservers,//filter by created @ object host
                       &unservers);//filter by destroyed @ object host
        MessageLatencyAnalysis::Filters nilfilter;
        MessageLatencyAnalysis::Filters pingfilter(&ping_port);
        MessageLatencyAnalysis la(STATS_TRACE_FILE,nservers,pingfilter);
        exit(0);
    }
    else if ( GetOption(ANALYSIS_BANDWIDTH)->as<bool>() ) {
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

        ba.computeJFI(server_id);
        exit(0);
    }
    else if ( !GetOption(ANALYSIS_WINDOWED_BANDWIDTH)->as<String>().empty() ) {
        String windowed_analysis_type = GetOption(ANALYSIS_WINDOWED_BANDWIDTH)->as<String>();

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

        Duration window = GetOption(ANALYSIS_WINDOWED_BANDWIDTH_WINDOW)->as<Duration>();
        Duration sample_rate = GetOption(ANALYSIS_WINDOWED_BANDWIDTH_RATE)->as<Duration>();
        BandwidthAnalysis ba(STATS_TRACE_FILE, max_space_servers);
        Time start_time = Time::null();
        Time end_time = start_time + duration;
        printf("Send rates\n");
        for(ServerID sender = 1; sender <= max_space_servers; sender++) {
            for(ServerID receiver = 1; receiver <= max_space_servers; receiver++) {
                if (windowed_analysis_type == "datagram")
                    ba.computeWindowedDatagramSendRate(sender, receiver, window, sample_rate, start_time, end_time, std::cout, windowed_analysis_send_file);
                else if (windowed_analysis_type == "packet")
                    ba.computeWindowedPacketSendRate(sender, receiver, window, sample_rate, start_time, end_time, std::cout, windowed_analysis_send_file);
            }
        }
        printf("Receive rates\n");
        for(ServerID sender = 1; sender <= max_space_servers; sender++) {
            for(ServerID receiver = 1; receiver <= max_space_servers; receiver++) {
                if (windowed_analysis_type == "datagram")
                    ba.computeWindowedDatagramReceiveRate(sender, receiver, window, sample_rate, start_time, end_time, std::cout, windowed_analysis_receive_file);
                else if (windowed_analysis_type == "packet")
                    ba.computeWindowedPacketReceiveRate(sender, receiver, window, sample_rate, start_time, end_time, std::cout, windowed_analysis_receive_file);
            }
        }
        // Queue information
        //  * Raw dump
        for(ServerID sender = 1; sender <= max_space_servers; sender++) {
            for(ServerID receiver = 1; receiver <= max_space_servers; receiver++) {
                if (windowed_analysis_type == "datagram")
                    ba.dumpDatagramQueueInfo(sender, receiver, std::cout, queue_info_file);
                else if (windowed_analysis_type == "packet")
                    ba.dumpPacketQueueInfo(sender, receiver, std::cout, queue_info_file);
            }
        }
        //  * Send
        for(ServerID sender = 1; sender <= max_space_servers; sender++) {
            for(ServerID receiver = 1; receiver <= max_space_servers; receiver++) {
                if (windowed_analysis_type == "datagram")
                    ba.windowedDatagramSendQueueInfo(sender, receiver, window, sample_rate, start_time, end_time, std::cout, windowed_queue_info_send_file);
                else if (windowed_analysis_type == "packet")
                    ba.windowedPacketSendQueueInfo(sender, receiver, window, sample_rate, start_time, end_time, std::cout, windowed_queue_info_send_file);
            }
        }
        //  * Receive
        for(ServerID sender = 1; sender <= max_space_servers; sender++) {
            for(ServerID receiver = 1; receiver <= max_space_servers; receiver++) {
                if (windowed_analysis_type == "datagram")
                    ba.windowedDatagramReceiveQueueInfo(sender, receiver, window, sample_rate, start_time, end_time, std::cout, windowed_queue_info_receive_file);
                else if (windowed_analysis_type == "packet")
                    ba.windowedPacketReceiveQueueInfo(sender, receiver, window, sample_rate, start_time, end_time, std::cout, windowed_queue_info_receive_file);
            }
        }

        exit(0);
    }
    else if ( GetOption(ANALYSIS_OSEG)->as<bool>() )
    {
      //bftm additional object messages log file creation.
      int osegProcessedAfterSeconds = GetOption(OSEG_ANALYZE_AFTER)->as<int>();


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
    else if ( GetOption(ANALYSIS_LOC_LATENCY)->as<bool>() ) {
        LocationLatencyAnalysis(STATS_TRACE_FILE, nservers);
        exit(0);
    }
    else if ( !GetOption(ANALYSIS_PROX_DUMP)->as<String>().empty() ) {
        ProximityDumpAnalysis(STATS_TRACE_FILE, nservers, GetOption(ANALYSIS_PROX_DUMP)->as<String>());
        exit(0);
    }

    gTrace->prepareShutdown();

    delete server_id_map;
    delete cseg;

    delete forwarder;

    gTrace->shutdown();
    delete gTrace;
    gTrace = NULL;

    delete space_context;
    space_context = NULL;

    delete mainStrand;
    IOServiceFactory::destroyIOService(ios);

    return 0;
}

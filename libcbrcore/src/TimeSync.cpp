/*  Sirikata
 *  TimeSync.cpp
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

#include <sirikata/cbrcore/TimeSync.hpp>
#include <sirikata/core/util/Timer.hpp>

#define STDIN 0
#define STDOUT 1

namespace Sirikata {

#define HEARTBEAT 'h'
#define KILLSYNC  'k'

#if SIRIKATA_PLATFORM != SIRIKATA_WINDOWS
void TimeSync_sync_thread(int ntp_ctl_pipes[], int ntp_data_pipes[], bool* synced, bool* done) {
        // Close the endpoints that the cbr process doesn't use
        close(ntp_ctl_pipes[STDIN]);
        close(ntp_data_pipes[STDOUT]);

        // Open the ctl pipe for writing
        FILE* ntp_ctl_fp = fdopen(ntp_ctl_pipes[STDOUT],"wb");
        // Open the ctl pipe for reading
        FILE* ntp_data_fp = fdopen(ntp_data_pipes[STDIN],"r");

        // Read off any remaining values
        float offset;
        while(*done == false) {
            int scanned = fscanf(ntp_data_fp, "%f", &offset);
            if (scanned == EOF) break;
            if (scanned == 0) {
                usleep(100000);
                continue;
            }
            //printf("Got offset %f\n", offset); fflush(stdout);
            // XXX sync only called once, after some testing we should allow it to set more often
            //if (*synced == false) {
                //printf("synced %f\n", offset); fflush(stdout);
                Timer::setSystemClockOffset(Duration::seconds((float64)offset));
                *synced = true;
            //}

            // Signal heartbeat to sync.py
            uint8 signalval = HEARTBEAT;
            fwrite(&signalval, sizeof(signalval), 1, ntp_ctl_fp);
            fflush(ntp_ctl_fp);
        }

        // Signal stop to sync.py
        uint8 signalval = KILLSYNC;
        fwrite(&signalval, sizeof(signalval), 1, ntp_ctl_fp);
        fflush(ntp_ctl_fp);

        // Close out pipes
        fclose(ntp_ctl_fp);
        close(ntp_ctl_pipes[STDOUT]);
        fclose(ntp_data_fp);
        close(ntp_data_pipes[STDIN]);
}
#endif

TimeSync::TimeSync()
 : mDone(false),
   mSyncThread(NULL)
{
}

void TimeSync::start(const String& server) {
#if SIRIKATA_PLATFORM == SIRIKATA_WINDOWS
	// FIXME #92
	mDone = true;
#else
    mDone = false;

    pipe(ntp_ctl_pipes);
    pipe(ntp_data_pipes);
    if (0==fork()) {
        // Close the endpoints that the sync script doesn't use
        close(ntp_ctl_pipes[STDOUT]);
        close(ntp_data_pipes[STDIN]);

        // Redirect stdin to the ctl pipe
        close(STDIN);
        dup2(ntp_ctl_pipes[STDIN], STDIN);

        // Redirect stdout to the data pipe
        close(STDOUT);
        dup2(ntp_data_pipes[STDOUT], STDOUT);

        // Run the actual sync script
        execlp("python","python","util/time_sync.py",server.c_str(),NULL);
    }
    {
        mSyncedOnce = false;

        // Start off the syncing thread
        mSyncThread = new Thread(
            std::tr1::bind(
            TimeSync_sync_thread,
            ntp_ctl_pipes,
            ntp_data_pipes,
            &mSyncedOnce,
            &mDone
            )
        );

        // Wait for it to sync at least once
        while(mSyncedOnce == false)
            usleep(10000);
    }
#endif
}

void TimeSync::stop() {
    assert(mSyncThread != NULL);
    printf("Stopping sync and waiting for sync thread to exit...\n"); fflush(stdout);
    mDone = true;
    mSyncThread->join();
    printf("Sync thread exited...\n"); fflush(stdout);
}


} // namespace Sirikata

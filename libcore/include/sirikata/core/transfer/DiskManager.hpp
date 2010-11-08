/*  Sirikata Transfer
 *  DiskManager.hpp
 *
 *  Copyright (c) 2010, Jeff Terrace
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

#ifndef SIRIKATA_DiskManager_HPP__
#define SIRIKATA_DiskManager_HPP__

#include <sirikata/core/queue/ThreadSafeQueue.hpp>
#include <sirikata/core/util/Thread.hpp>
#include <sirikata/core/util/Singleton.hpp>

namespace Sirikata {
namespace Transfer {

class SIRIKATA_EXPORT DiskManager
    : public AutoSingleton<DiskManager> {

public:

    /*
     * Stores information about a single file or directory.
     *
     * Examples:
     *    1. /usr/bin/ls
     *          getPath() = /usr/bin
     *          getFullPath() = /usr/bin/ls
     *          getLeafName() = ls
     *          isDirectory() = false
     *          isFile() = true
     *    2. /home/person
     *          getPath() = /home
     *          getFullPath() = /home/person
     *          getLeafName() = person
     *          isDirectory() = true
     *          isFile() = false
     */
    class DiskFile {
        std::string getPath();
        std::string getFullPath();
        std::string getLeafName();
        bool isDirectory();
        bool isFile();
    };

    class DiskRequest {
    public:
        virtual void execute() = 0;

        virtual ~DiskRequest() {}
    };

    class ScanRequest : public DiskRequest {
    public:
        typedef std::vector<DiskFile> DirectoryListing;
        typedef std::tr1::function<void(
                    std::tr1::shared_ptr<DirectoryListing> dirListing
                )> ScanRequestCallback;

        ScanRequest(DiskFile path, ScanRequestCallback cb);
        void execute();
    private:
        ScanRequestCallback mCb;
    };

    DiskManager();

    DiskManager& getSingleton();
    void destroy();

private:
    ThreadSafeQueue<std::tr1::shared_ptr<DiskRequest> > mRequestQueue;

};

}
}

#endif /* SIRIKATA_DiskManager_HPP__ */

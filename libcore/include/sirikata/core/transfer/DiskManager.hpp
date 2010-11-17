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
#include <sirikata/core/transfer/TransferData.hpp>
#include <boost/filesystem.hpp>

namespace Sirikata {
namespace Transfer {

namespace fs = boost::filesystem;

namespace Filesystem {
    typedef boost::filesystem::path Path;
    typedef boost::filesystem::file_status FileStatus;
    typedef boost::filesystem::file_type FileType;
    namespace boost_fs = boost::filesystem;
    class PathInfo {
    public:
        Path mPath;
        FileStatus mFileStatus;
        PathInfo(Path p, FileStatus f) : mPath(p), mFileStatus(f) {}
    };
}

class SIRIKATA_EXPORT DiskManager
    : public AutoSingleton<DiskManager> {

public:

    class DiskRequest {
    protected:
        virtual void execute() = 0;

        virtual ~DiskRequest() {}
    friend class DiskManager;
    };

    class SIRIKATA_EXPORT ScanRequest : public DiskRequest {
    public:
        typedef std::vector<Filesystem::PathInfo> DirectoryListing;
        typedef std::tr1::function<void(
                    std::tr1::shared_ptr<DirectoryListing> dirListing
                )> ScanRequestCallback;

        ScanRequest(Filesystem::Path path, ScanRequestCallback cb);
    private:
        ScanRequestCallback mCb;
        Filesystem::Path mPath;
    protected:
        void execute();
    };

    class SIRIKATA_EXPORT ReadRequest : public DiskRequest {
    public:
        typedef std::tr1::function<void(
                    std::tr1::shared_ptr<DenseData> fileContents
                )> ReadRequestCallback;

        ReadRequest(Filesystem::Path path, ReadRequestCallback cb);
    private:
        ReadRequestCallback mCb;
        Filesystem::Path mPath;
    protected:
        void execute();
    };

    class SIRIKATA_EXPORT WriteRequest : public DiskRequest {
    public:
        typedef std::tr1::function<void(
                    bool status
                )> WriteRequestCallback;

        WriteRequest(Filesystem::Path path, std::tr1::shared_ptr<DenseData> fileContents, WriteRequestCallback cb);
    private:
        WriteRequestCallback mCb;
        Filesystem::Path mPath;
        std::tr1::shared_ptr<DenseData> mFileContents;
    protected:
        void execute();
    };

    DiskManager();
    ~DiskManager();

    void addRequest(std::tr1::shared_ptr<DiskRequest> req);

    static DiskManager& getSingleton();
    static void destroy();

private:
    ThreadSafeQueue<std::tr1::shared_ptr<DiskRequest> > mRequestQueue;
    Thread *mWorkerThread;
    boost::mutex destroyLock;
    boost::condition_variable destroyCV;

    void workerThread();

};

}
}

#endif /* SIRIKATA_DiskManager_HPP__ */

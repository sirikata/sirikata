/*  Sirikata Transfer -- Content Distribution Network
 *  HTTPDownloadHandler.hpp
 *
 *  Copyright (c) 2009, Patrick Reiter Horn
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
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, class FileData;
, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SIRIKATA_FileProtocolHandler_HPP__
#define SIRIKATA_FileProtocolHandler_HPP__

#include "task/WorkQueue.hpp"
#include "DownloadHandler.hpp"
#include "UploadHandler.hpp"

namespace Sirikata {
namespace Transfer {

class SIRIKATA_EXPORT FileProtocolHandler :
		public DownloadHandler,
		public UploadHandler,
		public std::tr1::enable_shared_from_this<FileProtocolHandler>
{
	Task::WorkQueue *mQueue;
	Task::WorkQueueThread *mWorkerOwnsQueue;

	template <class Base> class FileData;
public:

	/** Constructs a FileProtocolHandler.
		It uses a WorkQueue to fetch items from disk in the background.
		@param queue A managed workQueue. If NULL, FileProtocolHandler will
					 create its own background thread.
	 */
	FileProtocolHandler(Task::WorkQueue *queue=NULL);

	~FileProtocolHandler();

	static std::string pathFromURI(const URI &uri);
	Task::WorkQueue *getWorkQueue() const {
		return mQueue;
	}

	virtual void download(DownloadHandler::TransferDataPtr *ptrRef,
			const URI &uri,
			const Range &bytes,
			const DownloadHandler::Callback &cb);

	virtual void stream(DownloadHandler::TransferDataPtr *ptrRef,
			const URI &uri,
			const Range &bytes,
			const DownloadHandler::Callback &cb);
	virtual bool inOrderStream() const {
		return true;
	}
	virtual void exists(DownloadHandler::TransferDataPtr *ptrRef,
			const URI &uri,
			const DownloadHandler::Callback &cb);

	virtual void upload(UploadHandler::TransferDataPtr *ptrRef,
			const ServiceParams &params,
			const URI &uri,
			const DenseDataPtr &uploadData,
			const UploadHandler::Callback &cb);
	virtual void remove(UploadHandler::TransferDataPtr *ptrRef,
			const ServiceParams &params,
			const URI &uri,
			const UploadHandler::Callback &cb);
	void append(UploadHandler::TransferDataPtr *ptrRef,
			const URI &uri,
			const std::string &uploadData,
			const UploadHandler::Callback &cb);

};
typedef std::tr1::shared_ptr<FileProtocolHandler> FileProtocolHandlerPtr;

/** Stores a file-name log. of mappings from names to hashes. */
class SIRIKATA_EXPORT FileNameHandler :
		public NameLookupHandler,
		public NameUploadHandler,
		public std::tr1::enable_shared_from_this<FileNameHandler>
{
	FileProtocolHandlerPtr mFileProtocol;

	struct NameLookupProgress : public ProtocolData<NameLookupHandler> {
		DownloadHandler::TransferDataPtr mProgress;
		NameLookupProgress(NameLookupHandlerPtr parent) 
			:ProtocolData<NameLookupHandler>(parent) {
		}
		virtual ~NameLookupProgress() {}
		virtual void abort() {
			mProgress->abort();
		}
	};
	struct NameUploadProgress : public ProtocolData<NameUploadHandler> {
		UploadHandler::TransferDataPtr mProgress;
		NameUploadProgress(NameUploadHandlerPtr parent) 
			:ProtocolData<NameUploadHandler>(parent) {
		}
		virtual ~NameUploadProgress() {}
		virtual void abort() {
			mProgress->abort();
		}
	};

	void finishedDownload(const NameLookupHandler::Callback &cb,
						  const std::string &filename,
						  DenseDataPtr data,
						  bool success);
public:
	FileNameHandler(FileProtocolHandlerPtr fileProtocol)
		: mFileProtocol(fileProtocol) {
	}

	virtual void nameLookup(NameLookupHandler::TransferDataPtr *ptrRef,
			const URI &uri,
			const NameLookupHandler::Callback &cb) {
		std::tr1::shared_ptr<NameLookupProgress> data(new NameLookupProgress(shared_from_this()));
        if (ptrRef) {
            *ptrRef = NameLookupHandler::TransferDataPtr(data);
        }
		mFileProtocol->download(&data->mProgress, URI(uri.context().toString(false)), Range(true),
								std::tr1::bind(&FileNameHandler::finishedDownload, this, cb, uri.filename(), _1, _2));
	}

	virtual void uploadName(NameUploadHandler::TransferDataPtr *ptrRef,
			const ServiceParams &params,
			const URI &uri,
			const RemoteFileId &uploadId,
			const NameUploadHandler::Callback &cb) {
		std::tr1::shared_ptr<NameUploadProgress> data(new NameUploadProgress(shared_from_this()));
        if (ptrRef) {
            *ptrRef = NameUploadHandler::TransferDataPtr(data);
        }
		mFileProtocol->append(&data->mProgress, URI(uri.context().toString(false)),
							  uri.filename() + " " + uploadId.uri().toString() + "\n", cb);
	}

	virtual void removeName(NameUploadHandler::TransferDataPtr *ptrRef,
			const ServiceParams &params,
			const URI &uri,
			const NameUploadHandler::Callback &cb) {
		std::tr1::shared_ptr<NameUploadProgress> data(new NameUploadProgress(shared_from_this()));
        if (ptrRef) {
            *ptrRef = NameUploadHandler::TransferDataPtr(data);
        }
		mFileProtocol->append(&data->mProgress, URI(uri.context().toString(false)),
                              uri.filename()+"\n", cb);
	}
};

}
}

#endif

/*  Sirikata Transfer -- Content Distribution Network
 *  FileProtocolHandler.hpp
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
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "util/Standard.hh"
#include "ServiceLookup.hpp"
#include "FileProtocolHandler.hpp"
#include "util/LockFreeQueue.hpp"
#include "util/ThreadSafeQueue.hpp"
#include "task/WorkQueue.hpp"

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifndef _WIN32
#ifdef __APPLE__
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#define fstat64 fstat
#define stat64 stat
#define lseek64 lseek
#else
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#endif
#define O_BINARY 0 // Other OS's don't always define this flag.
#else
#include <io.h>
#include <direct.h>
#include <fcntl.h>
// Lovely underscores to obscure the beauty of Posix
#define fstat64 _fstat64
#define stat64 _stat64
#define open _open
#define close _close
#define read _read
#define write _write
#define lseek _lseeki64
#define lseek64 _lseeki64
#define seek _seeki64 // by default, not 64-bit seek
#define mkdir _mkdir
#define unlink _unlink
#define access _access
#define O_RDONLY _O_RDONLY
#define O_WRONLY _O_WRONLY
#define O_CREAT _O_CREAT
#define O_BINARY _O_BINARY
#define F_OK 0
#define W_OK 2
#define R_OK 4
#define _finddata64_t __finddata64_t
#endif

namespace Sirikata {
namespace Transfer {



static String SystemAdministratorString("$ystem Administrator");


template <class Base> class FileProtocolHandler::FileData : public ProtocolData<Base> {
	Task::AbortableWorkItemPtr mWork;
	DownloadHandler::Callback cb;
public:
	FileData(const std::tr1::shared_ptr<Base> &parent,
			 const Task::AbortableWorkItemPtr &workItem)
		: ProtocolData<Base>(parent),
		  mWork(workItem) {
	}
	virtual ~FileData() {}
	virtual void abort() {
		mWork->abort();
	}
};

FileProtocolHandler::FileProtocolHandler(Task::WorkQueue *queue) {
	if (queue) {
		mQueue = queue;
		mWorkerOwnsQueue = NULL;
	} else {
		mQueue = new Task::LockFreeWorkQueue;
		mWorkerOwnsQueue = mQueue->createWorkerThreads(2); // does not guarantee ordering.
	}
}

FileProtocolHandler::~FileProtocolHandler() {
	Task::WorkQueue *workQueue = mQueue;
	mQueue = NULL; // prevent adding new jobs in response to callbacks.
	if (mWorkerOwnsQueue) {
		workQueue->destroyWorkerThreads(mWorkerOwnsQueue);
		delete workQueue;
	}
}

bool read_full(int fd, unsigned char *data, cache_ssize_type amount) {
	cache_ssize_type ret;
 	while (amount > 0) {
		ret = read(fd, data, amount);
		if (ret <= 0) {
			if (ret == 0 || (errno != EAGAIN && errno != EINTR)) {
				// ret == 0 means end-of-file.
				return false; // read error before finished!
			}
			continue;
		}
		amount -= ret;
		data += ret;
	}
	return true; // read all data.
}

bool write_full(int fd, const unsigned char *data, cache_ssize_type amount) {
	cache_ssize_type ret;
 	while (amount > 0) {
		ret = write(fd, data, amount);
		if (ret <= 0) {
			if (ret == 0 || (errno != EAGAIN && errno != EINTR)) {
				// ret == 0 means end-of-file.
				return false; // write error before finished!
			}
			continue;
		}
		amount -= ret;
		data += ret;
	}
	return true; // written all data.
}

int open_at_byte(const char *path, int options, cache_usize_type startbyte=0, int mode=0664) {
	int fd = open(path, options|O_BINARY, mode);
	if (fd < 0) {
		SILOG(transfer,error, "Failed to open " << path <<
			  " for "<<options<<"; reason: " << errno);
		return -1;
	}
	if (startbyte != 0) {
		// FIXME: may not work with 64-bit files?
		if (lseek(fd, startbyte, SEEK_SET) != (cache_ssize_type)startbyte) {
			SILOG(transfer,error, "Failed to seek in " << path <<
				  " to byte "<<startbyte<<"; reason: " << errno);
			return -1;
		}
	}
	return fd;
}

class ExistsTask : public Task::AbortableWorkItem {
protected:
	std::string mPath;
	DownloadHandler::Callback mCallback;
public:
	ExistsTask(const std::string &path, const DownloadHandler::Callback &cb) 
		: mPath(path), mCallback(cb) {
	}

	void operator() () {
		Task::AbortableWorkItemPtr tempReference(shared_from_this());
		if (!prepareExecute()) {
			return;
		}
		struct stat64 st;
		cache_usize_type diskSize = 0;
		if (stat64(mPath.c_str(), &st)!=0) {
			aborted();
			return;
		}
		if (st.st_size > 0) {
			diskSize = st.st_size;
		}
		if (access(mPath.c_str(), R_OK)==0) {
			mCallback(DenseDataPtr(), true, diskSize);
		} else {
			mCallback(DenseDataPtr(), false, diskSize);
		}
	}

	virtual void aborted() {
		mCallback(DenseDataPtr(), false, 0);
	}
};

class ReadTask : public Task::AbortableWorkItem {
protected:
	std::string mPath;
	Range mRange;
	DownloadHandler::Callback mCallback;
	int mFd;
	cache_usize_type mDiskSize;
public:
	ReadTask(const std::string &path, const Range &bytes, const DownloadHandler::Callback &cb) 
		: mPath(path), mRange(bytes), mCallback(cb), mFd(-1), mDiskSize(0) {
	}

	virtual void readLoop() {
		if (mRange.goesToEndOfFile()) {
			if (mDiskSize > 0) {
				mRange.setLength(mDiskSize - mRange.startbyte(), true);
			} else {
				aborted();
				return;
			}
		}
		MutableDenseDataPtr memoryBuffer(new DenseData(mRange));
		if (!read_full(mFd, memoryBuffer->writableData(), mRange.length())) {
			aborted();
			return;
		}
		mCallback(memoryBuffer, true, mDiskSize);
	}

	void operator() () {
		Task::AbortableWorkItemPtr tempReference(shared_from_this());
		if (!prepareExecute()) {
			return;
		}

		mFd = open_at_byte(mPath.c_str(), O_RDONLY, mRange.startbyte());
		if (mFd == -1) {
			aborted();
			return;
		}
		struct stat64 st;
		if (fstat64(mFd, &st)==0 && st.st_size > 0) {
			mDiskSize = st.st_size;
		}

		readLoop();
		close(mFd);
	}

	virtual bool abort() {
		if (!Task::AbortableWorkItem::abort()) {
			close(mFd);
		}
		return true;
	}
	virtual void aborted() {
		mCallback(DenseDataPtr(), false, mDiskSize);
	}
};

static const int READ_SIZE = 4096;

// FIXME: If we are "streaming" from a pipe or a device, this function may never return.
// This will cause the queue to build up.
// We need some way to keep all of our disk accesses around by using select.
// For now, assume the file is of finite size and not on a stalled NFS drive.

class StreamTask : public ReadTask {
public:
	StreamTask(const std::string &path, const Range &bytes, const DownloadHandler::Callback &cb) 
		: ReadTask(path, bytes, cb) {
	}
	virtual void readLoop(float size) {
		cache_usize_type pos = mRange.startbyte();
		while (mRange.goesToEndOfFile() || pos < mRange.endbyte()) {
			char temporary_buffer[READ_SIZE];
			cache_ssize_type amount_read = READ_SIZE;
			if ((cache_ssize_type)(mRange.endbyte() - pos) < READ_SIZE) {
				amount_read = (mRange.endbyte() - pos);
			}
			amount_read = read(mFd, temporary_buffer, READ_SIZE);
			if (amount_read <= 0) {
				if (amount_read == 0 || (errno != EAGAIN && errno != EINTR)) {
					break;
				}
				continue;
			}
			MutableDenseDataPtr mddp (new DenseData(Range(pos, amount_read, LENGTH)));
			memcpy(mddp->writableData(), temporary_buffer, amount_read);
			mCallback(mddp, true, mDiskSize);
			pos += amount_read;
		}
		aborted();
	}
};

void makeParentDir(const std::string &openPath) {
	std::string::size_type lastslash = openPath.rfind('/');
	if (lastslash != std::string::npos) {
		std::string dirname = openPath.substr(0, lastslash);
		mkdir(dirname.c_str()
#ifndef _WIN32
			, 0775
#endif
			);
	}
}

class WriteTask : public Task::AbortableWorkItem {
	std::string mPath;
	std::string mTemporaryPath;
	UploadHandler::Callback mCallback;
	int mFd;
	SparseData mData;
	bool mAppendMode;
public:
	WriteTask(const std::string &path,
			std::string temporaryPath,
			SparseData data,
			const UploadHandler::Callback &cb,
			bool appendMode = false) 
		: mPath(path), mTemporaryPath(temporaryPath), mCallback(cb), mFd(-1), mData(data), mAppendMode(appendMode) {
	}
	void operator() () {
		Task::AbortableWorkItemPtr tempReference(shared_from_this());
		if (!prepareExecute()) {
			return;
		}
		int flags;
		std::string openPath;
		if (!mTemporaryPath.empty()) {
			flags = O_CREAT|O_TRUNC|O_WRONLY;
			openPath = mTemporaryPath;
		} else {
			flags = O_CREAT|O_WRONLY;
			openPath = mPath;
		}
		if (mAppendMode && mTemporaryPath.empty()) {
			flags |= O_APPEND;
		}
		makeParentDir(openPath);
		mFd = open_at_byte(openPath.c_str(), flags, mData.startbyte());
		if (mFd == -1) {
			aborted();
			return;
		}
		cache_usize_type pos = mData.startbyte(), length = 0;
		do {
			const unsigned char *data = mData.dataAt(pos, length);
			if (!data) {
				lseek64(mFd, SEEK_CUR, length);
			} else {
				if (!write_full(mFd, data, length)) {
					aborted();
					return;
				}
			}
			pos += length;
		} while (length);
		if (!mTemporaryPath.empty()) {
			// Fixme: Not guaranteed to be ordered with write() calls, google for ext4 data loss.
			makeParentDir(mPath);
			rename(mTemporaryPath.c_str(), mPath.c_str());
		}
		mCallback(true);
		close(mFd);
	}
	virtual void aborted() {
		mCallback(false);
	}
};

class UnlinkTask : public Task::AbortableWorkItem {
	std::string mPath;
	UploadHandler::Callback mCallback;
public:
	UnlinkTask(const std::string &path, const UploadHandler::Callback &cb)
		:mPath(path), mCallback(cb) {
	}
	virtual void operator()() {
		Task::AbortableWorkItemPtr tempReference(shared_from_this());
		if (!prepareExecute()) {
			return;
		}
		unlink(mPath.c_str());
		mCallback(true);
	}

	virtual bool abort() {
		if (!Task::AbortableWorkItem::abort()) {
			mCallback(false);
		}
		return true;
	}
};


std::string FileProtocolHandler::pathFromURI(const URI &path) {
	return path.fullpath().substr(1); // exclude '/'.
}

void FileProtocolHandler::download(DownloadHandler::TransferDataPtr *ptrRef,
		const URI &uri,
		const Range &bytes,
		const DownloadHandler::Callback &cb) {
	Task::AbortableWorkItemPtr work(new ReadTask(pathFromURI(uri), bytes, cb));
	if (ptrRef) {
		*ptrRef = DownloadHandler::TransferDataPtr(
			new FileData<DownloadHandler>(shared_from_this(), work));
	}
	mQueue->enqueue(&*work);
}

void FileProtocolHandler::stream(DownloadHandler::TransferDataPtr *ptrRef,
		const URI &uri,
		const Range &bytes,
		const DownloadHandler::Callback &cb) {
	Task::AbortableWorkItemPtr work(new StreamTask(pathFromURI(uri), bytes, cb));
	if (ptrRef) {
		*ptrRef = DownloadHandler::TransferDataPtr(
			new FileData<DownloadHandler>(shared_from_this(), work));
	}
	mQueue->enqueue(&*work);
}

void FileProtocolHandler::exists(DownloadHandler::TransferDataPtr *ptrRef,
		const URI &uri,
		const DownloadHandler::Callback &cb) {
	Task::AbortableWorkItemPtr work(new ExistsTask(pathFromURI(uri), cb));
	if (ptrRef) {
		*ptrRef = DownloadHandler::TransferDataPtr(
			new FileData<DownloadHandler>(shared_from_this(), work));
	}
	mQueue->enqueue(&*work);
}


void FileProtocolHandler::upload(UploadHandler::TransferDataPtr *ptrRef,
		const ServiceParams &params,
		const URI &uri,
		const DenseDataPtr &uploadData,
		const UploadHandler::Callback &cb) {
	std::string path(pathFromURI(uri));
	Task::AbortableWorkItemPtr work(new WriteTask(path, path+".tmp", uploadData, cb));
	if (ptrRef) {
		*ptrRef = UploadHandler::TransferDataPtr(
			new FileData<UploadHandler>(shared_from_this(), work));
	}
	mQueue->enqueue(&*work);
}

void FileProtocolHandler::append(UploadHandler::TransferDataPtr *ptrRef,
		const URI &uri,
		const std::string &appendStr,
		const UploadHandler::Callback &cb) {
	std::string path(pathFromURI(uri));
	DenseDataPtr uploadData (new DenseData(appendStr));
	Task::AbortableWorkItemPtr work(new WriteTask(path, std::string(), uploadData, cb, true));
	if (ptrRef) {
		*ptrRef = UploadHandler::TransferDataPtr(
			new FileData<UploadHandler>(shared_from_this(), work));
	}
	mQueue->enqueue(&*work);
}

void FileProtocolHandler::remove(UploadHandler::TransferDataPtr *ptrRef,
		const ServiceParams &params,
		const URI &uri,
		const UploadHandler::Callback &cb) {
	std::string todelete (pathFromURI(uri));
	unlink(todelete.c_str());
}


void FileNameHandler::finishedDownload(const NameLookupHandler::Callback &cb,
	const std::string &filename, DenseDataPtr data, bool success)
{
	size_t pos;
	bool exists = false;
	RemoteFileId foundURI;
	if (success) {
		for (const unsigned char *iter = data->begin(); iter != data->end();) {
			const unsigned char *newlinepos = std::find(iter, data->end(), '\n');
			if (newlinepos == data->end()) {
				break;
			}
			const unsigned char *spacepos = std::find(iter, newlinepos, ' ');
			if (std::string(iter, spacepos) != filename) {
				iter = newlinepos + 1;
				continue;
			}
			if (spacepos == newlinepos || spacepos + 1 == newlinepos) {
				// the name does not exist.
				exists = false;
			} else {
				exists = true;
				foundURI = RemoteFileId(URI(std::string(spacepos+1, newlinepos)));
			}
			iter = newlinepos + 1;
		}
	}
	if (exists) {
		cb(foundURI.fingerprint(), foundURI.uri().toString(), true);
	} else {
		cb(Fingerprint::null(), std::string(), false);
	}
}


}
}

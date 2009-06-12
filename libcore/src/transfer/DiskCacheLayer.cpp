/*  Sirikata Transfer -- Content Transfer management system
 *  DiskCacheLayer.cpp
 *
 *  Copyright (c) 2008, Patrick Reiter Horn
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
/*  Created on: Jan 15, 2009 */
#include "util/Standard.hh"
#include "DiskCacheLayer.hpp"

#include "options/Options.hpp"
#include <sys/types.h>
#include <sys/stat.h>

#ifndef _WIN32
#ifdef __APPLE__
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#define fstat64 fstat
#define stat64 stat
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
#define seek _seeki64 // by default, not 64-bit seek
#define mkdir _mkdir
#define unlink _unlink
#define O_RDONLY _O_RDONLY
#define O_WRONLY _O_WRONLY
#define O_CREAT _O_CREAT
#define O_BINARY _O_BINARY
#define _finddata64_t __finddata64_t
#endif

#define DEFAULT_OPEN_OPTIONS O_BINARY

namespace Sirikata {

namespace Transfer {

static const char *PARTIAL_SUFFIX = ".part";
static const char *RANGES_SUFFIX = ".ranges";

namespace {

cache_usize_type getDiskUsage(const struct stat64 *st) {
#ifdef _WIN32
	return (cache_usize_type)st->st_size;
#else
	// http://lkml.indiana.edu/hypermail/linux/kernel/9812.2/0216.html
	return 512 * (cache_usize_type)st->st_blocks;
#endif
}

#ifndef _WIN32

cache_usize_type sizeFromDirentry(const std::string &name, dirent *ent, bool &isdir) {
	std::string temp = name;
	struct stat64 mystat;
	if (stat64(temp.c_str(), &mystat) != 0) {
		perror(temp.c_str());
		isdir = true; // to ignore.
		return 0;
	}
	isdir = ((mystat.st_mode & S_IFDIR) == S_IFDIR);
	return getDiskUsage(&mystat);
}

#else

struct dirent {
	char d_name[_MAX_PATH];
	cache_usize_type st_size;
	bool isdir;
};

struct DIR {
	struct _finddata64_t winentry;
	intptr_t handle;
	struct dirent myentry;
	bool finished;
};

DIR *opendir(const char *name) {
	std::string myName (name);
	for (std::string::size_type i = 0; i < myName.length(); ++i) {
		if (myName[i]=='/') {
			myName[i]='\\';
		}
	}
	/*
	if (myName[myName.length()-1]=='/' || myName[myName.length()-1]=='\\') {
		myName = myName.substr(0, myName.length()-1);
	}
	*/
	myName += "*";
	DIR *mydir = new DIR;
	char test[1024];
	getcwd(test,1024);
	mydir->handle = _findfirst64(myName.c_str(), &mydir->winentry);
	mydir->finished = false;
	if (mydir->handle == -1) {
		delete mydir;
		return NULL;
	}
	return mydir;
}

struct dirent *readdir(DIR *mydir) {
	if (mydir->finished) {
		return NULL;
	} else {
		strcpy(mydir->myentry.d_name, mydir->winentry.name);
		mydir->myentry.st_size = mydir->winentry.size;
		mydir->myentry.isdir = ((mydir->winentry.attrib & _A_SUBDIR) == _A_SUBDIR);
	}
	if (_findnext64(mydir->handle, &mydir->winentry) == -1) {
		mydir->finished = true;
	}
	return &mydir->myentry;
}

void closedir(DIR *mydir) {
	_findclose(mydir->handle);
	delete mydir;
}

cache_usize_type sizeFromDirentry(const std::string &name, dirent *ent, bool &isdir) {
	isdir = ent->isdir;
	return ent->st_size;
}

#endif

} // anon namespace.

void DiskCacheLayer::workerThread() {
	while (true) {
		std::tr1::shared_ptr<DiskRequest> req;

		mRequestQueue.blockingPop(req);
		if (req->op == DiskRequest::OPEXIT) {
			break;
		} else if (req->op == DiskRequest::OPWRITE) {
			// Note: TransferLayer::populatePreviousCaches has already been called.
			std::string fileId = req->fileId.fingerprint().convertToHexString();
			bool newFile = true;
			{
				CacheMap::write_iterator writer(mFiles);
				if (writer.find(req->fileId.fingerprint())) {
					CacheData *rlist = static_cast<CacheData*>(*writer);
					if (rlist->wholeFile() || rlist->contains(*(req->data))) {
						// this range is already written to disk.
						continue;
					}
					newFile = false;
				}
				if (!mFiles.alloc(req->data->length(), writer)) {
					continue;
				}
			}

			std::string rangesPath = mPrefix + fileId + RANGES_SUFFIX;
			std::string filePath = mPrefix + fileId + PARTIAL_SUFFIX;
			if (newFile) {
				unlink(rangesPath.c_str()); // in case of a leftover old file.
			}
			int fd = open(filePath.c_str(), O_CREAT|O_WRONLY|DEFAULT_OPEN_OPTIONS, 0666);
			if (fd < 0) {
				SILOG(transfer,error, "Failed to open " << fileId <<
					"for writing; reason: " << errno);
				continue;
			}
			lseek(fd, req->data->startbyte(), SEEK_SET);
			write(fd, req->data->data(), (size_t)req->data->length());
			cache_usize_type diskUsage;
			{
				struct stat64 st;
				fstat64(fd, &st);
				diskUsage = getDiskUsage(&st);
			}
			close(fd);

			std::string rangesStr;
			{
				CacheMap::write_iterator writer(mFiles);

				if (writer.insert(req->fileId.fingerprint(), diskUsage)) {
					*writer = new CacheData;
					writer.use();
				} else {
					writer.update(diskUsage);
				}
				RangeList &data = static_cast<CacheData*>(*writer)->mRanges;
				req->data->addToList(*(req->data), data);
				if (Range(true).isContainedBy(data)) {
					data.clear();
				} else {
					serializeRanges(data, rangesStr);
				}
			}

			if (rangesStr.empty()) {
				std::string renameToPath = mPrefix + fileId;
				// first do atomic rename, the delete ranges file.
				rename(filePath.c_str(), renameToPath.c_str());
				unlink(rangesPath.c_str());
			} else {
				std::string rangesTempPath = rangesPath + ".temp";
				FILE * fp = fopen(rangesTempPath.c_str(), "wb");
				fwrite(rangesStr.data(), 1, rangesStr.length(), fp);
				fclose(fp);
				rename(rangesTempPath.c_str(), rangesPath.c_str());
			}
		} else if (req->op == DiskRequest::OPREAD) {
			bool useWholeFile = false;
			{
				CacheMap::read_iterator iter(mFiles);
				if (iter.find(req->fileId.fingerprint())) {
					CacheData *rlist = static_cast<CacheData*>(*iter);
					if (rlist->wholeFile()) {
						useWholeFile = true;
					} else if (!rlist->contains(req->toRead)) {
						// this range is already written to disk.
						CacheLayer::getData(req->fileId, req->toRead, req->finished);
						continue;
					}
				}
			}
			std::string fileId = req->fileId.fingerprint().convertToHexString();
			std::string filePath = mPrefix + fileId;
			if (!useWholeFile) {
				filePath += PARTIAL_SUFFIX;
			}
			int fd = open(filePath.c_str(), O_RDONLY|DEFAULT_OPEN_OPTIONS);
			if (fd < 0) {
				SILOG(transfer,error, "Failed to open " << fileId <<
					"for writing; reason: " << errno);
				CacheLayer::getData(req->fileId, req->toRead, req->finished);
				continue;
			}
			if (req->toRead.goesToEndOfFile()) {
				struct stat64 st;
				if (fstat64(fd, &st)==0 && st.st_size > 0) {
					req->toRead.setLength(st.st_size - req->toRead.startbyte(), true);
				}
			}
			if (req->toRead.startbyte() != 0) {
				// FIXME: may not work with 64-bit files?
				if (lseek(fd, req->toRead.startbyte(), SEEK_SET) != (cache_ssize_type)req->toRead.startbyte()) {
					SILOG(transfer,error, "Failed to seek in " << fileId <<
						"to byte "<<req->toRead.startbyte()<<"; reason: " << errno);
					CacheLayer::getData(req->fileId, req->toRead, req->finished);
					continue;
				}
			}
			MutableDenseDataPtr datum(new DenseData(req->toRead));
			read(fd, datum->writableData(), (size_t)req->toRead.length());
			close(fd);

			CacheLayer::populateParentCaches(req->fileId.fingerprint(), datum);
			SparseData data;
			data.addValidData(datum);
			req->finished(&data);
		} else if (req->op == DiskRequest::OPDELETE) {
			std::string fileId = req->fileId.fingerprint().convertToHexString();
			std::string filePath = mPrefix + fileId;
			unlink(filePath.c_str());
			std::string rangesPath = filePath + RANGES_SUFFIX;
			unlink(rangesPath.c_str());
			std::string partialPath = filePath + PARTIAL_SUFFIX;
			unlink(partialPath.c_str());
		}
	}
	{
		boost::unique_lock<boost::mutex> wake_cv(destroyLock);
		destroyCV.notify_one();
	}
}

void DiskCacheLayer::unserialize() {
	std::string::size_type slash=0;
	while (true) {
		std::string::size_type fwdslash = mPrefix.find('/', slash);
		std::string::size_type backslash = mPrefix.find('\\', slash);
		if (fwdslash == std::string::npos && backslash == std::string::npos) {
			break;
		}
		if (fwdslash == std::string::npos) {
			slash = backslash;
		} else if (backslash == std::string::npos) {
			slash = fwdslash;
		} else {
			slash = fwdslash<backslash ? fwdslash : backslash;
		}
		std::string thisDir = mPrefix.substr(0, slash);
		mkdir(thisDir.c_str()
#ifndef _WIN32
					,0755
#endif
					);

		++slash;
	}

	DIR *mydir = opendir (mPrefix.c_str());
	if(mydir) {
		dirent *myentry;
		CacheMap::write_iterator writer (mFiles);
		while ((myentry = readdir(mydir)) != NULL) {
			cache_usize_type totalLength;
			std::string strName (myentry->d_name);
			std::string pathName (mPrefix + strName);
			bool isdir = false;
			if (strName.length() > strlen(RANGES_SUFFIX) &&
					strName.substr(strName.length()-strlen(RANGES_SUFFIX)) == RANGES_SUFFIX) {
				continue; // will find range files later.
			}
			totalLength = sizeFromDirentry(pathName, myentry, isdir);
			if (isdir) {
				continue; // ignore directories (including . and ..)
			}

			CacheData *cdata = new CacheData();
			std::string fingerprintName(strName);
			bool thisispartial = false;
			if (strName.length() > strlen(PARTIAL_SUFFIX) &&
					strName.substr(strName.length()-strlen(PARTIAL_SUFFIX)) == PARTIAL_SUFFIX) {
				thisispartial = true;
				fingerprintName = strName.substr(0, strName.length()-strlen(PARTIAL_SUFFIX));

				std::string rangeFile (mPrefix + fingerprintName + RANGES_SUFFIX);
				std::fstream fp (rangeFile.c_str(), std::ios_base::in);
				unserializeRanges(cdata->mRanges, fp);
				if (cdata->mRanges.empty()) {
					unlink(rangeFile.c_str());
					unlink(pathName.c_str());
					delete cdata;
					continue; // failed to read ranges file -> ignore.
				}
			}

			SILOG(transfer,debug,"Cached fingerprint: " << fingerprintName <<
				"(" << totalLength << ")");

			Fingerprint fprint;
			try {
				fprint = SHA256::convertFromHex(fingerprintName);
			} catch (std::invalid_argument) {
				// Invalid filename, not a fingerprint.
				delete cdata;
				continue;
			}

			if (!writer.insert(fprint, totalLength)) {
				delete cdata;
				cdata = NULL;
				if (!thisispartial) {
					/* We earlier found a partial file with its associated
					  ranges... but now we found the whole file as well.
					  FIXME: is this case possible? worth worrying about?
					 */
					cdata = static_cast<CacheData*>(*writer);
					cdata->mRanges.clear(); // we found a complete file.
					// a complete file overrides partial ones.
					std::string partialName(pathName + PARTIAL_SUFFIX);
					unlink(partialName.c_str());
				}
				continue;
			}

			*writer = cdata; // Finally, set the iterator contents.
		}
		closedir(mydir);
		// And we are done reading the directory.
	}
}

}
}

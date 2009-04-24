/*  Meru
 *  ResourceTransfer.hpp
 *
 *  Copyright (c) 2009, Stanford University
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
#ifndef _RESOURCE_TRANSFER_HPP_
#define _RESOURCE_TRANSFER_HPP_

#include "Event.hpp"

//below for ResourceBuffer
#include "ResourceManager.hpp"

namespace Meru {

/** Base class representing an in-progress resource transfer, either upload
 *  or download.
 */
class ResourceTransfer {
public:
	/** Create a ResourceTransfer for the given resource.
	 *  \param rid the ResourceID of the resource to be transferred
	 */
	ResourceTransfer(const URI& rid);
	virtual ~ResourceTransfer();

	/** Returns true if the transfer has completed. */
	bool complete() const;
	/** Returns true if the transfer has completed successfully. */
	bool succeeded() const;
	/** Returns the ResourceID of the resource being transferred. */
	const URI& id() const;

	/** Starts transferring the resource. */
	virtual void startTransfer() {}

	/** Signal that the transfer has completed. */
	bool operator == (const ResourceTransfer&rd) const{
		return rd.id() == mID;
	}
	bool operator != (const ResourceTransfer&rd) const{
		return rd.id() != mID;
	}
	bool operator < (const ResourceTransfer&rd) const{
		return mID < rd.id();
	}
protected:
	const URI mID;
	bool mFinished;
	bool mSucceeded;
};


/** Functor to check equality of ResourceIDs */
struct EqualResourceID {
	EqualResourceID(const URI& rid) : mID(rid) {}

	bool operator()(const ResourceTransfer* rd) {
		return (rd->id() == mID);
	}

	const URI mID;
};



namespace EventTypes {
	extern const EventType ResourceUploadEvent;
	extern const EventType ResourceDownloadEvent;
} // namespace EventTypes

class TransferEvent : public Event {
public:
	TransferEvent(const EventType& type, const URI& rid, bool success)
		: Event(EventID(type, rid.toString())), mID(rid),mSuccess(success) {}
    const bool success() const {return mSuccess;}
	const URI& resourceID() const { return mID; }
private:
	URI mID;
    bool mSuccess;
};
typedef std::tr1::shared_ptr<TransferEvent> TransferEventPtr;

class UploadCompleteEvent : public TransferEvent {
public:
	UploadCompleteEvent(const URI& rid)
		: TransferEvent(EventTypes::ResourceUploadEvent, rid,true) {}
};
typedef std::tr1::shared_ptr<UploadCompleteEvent> UploadCompleteEventPtr;

class UploadFailedEvent : public TransferEvent {
public:
	UploadFailedEvent(const URI& rid)
		: TransferEvent(EventTypes::ResourceUploadEvent, rid,false) {}
};
typedef std::tr1::shared_ptr<UploadFailedEvent> UploadFailedEventPtr;

class DownloadCompleteEvent : public TransferEvent {
public:
	DownloadCompleteEvent(const URI& rid,ResourceBuffer rbuffer)
		: TransferEvent(EventTypes::ResourceDownloadEvent, rid,true),mResourceBuffer(rbuffer) {}
    ResourceBuffer mResourceBuffer;
};
typedef std::tr1::shared_ptr<DownloadCompleteEvent> DownloadCompleteEventPtr;

class DownloadFailedEvent : public TransferEvent {
public:
	DownloadFailedEvent(const URI& rid)
		: TransferEvent(EventTypes::ResourceDownloadEvent, rid,false) {
        if (rid.toString().length()==0)    {
            rid.toString();
        }
    }
};
typedef std::tr1::shared_ptr<DownloadFailedEvent> DownloadFailedEventPtr;

} // namespace Meru

#endif //_RESOURCE_TRANSFER_HPP_

/*  Sirikata Network Utilities
 *  IOService.cpp
 *
 *  Copyright (c) 2009, Daniel Reiter Horn
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

#include <sirikata/core/util/Standard.hh>
#include <sirikata/core/network/IOService.hpp>
#include <sirikata/core/util/Time.hpp>
#include <sirikata/core/network/IOStrand.hpp>
#include <boost/version.hpp>
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <sirikata/core/util/Timer.hpp>
#include <sirikata/core/command/Commander.hpp>

namespace Sirikata {
namespace Network {

typedef boost::asio::io_service InternalIOService;

typedef boost::asio::deadline_timer deadline_timer;
#if BOOST_VERSION==103900
typedef deadline_timer* deadline_timer_ptr;
#else
typedef std::tr1::shared_ptr<deadline_timer> deadline_timer_ptr;
#endif
typedef boost::posix_time::microseconds posix_microseconds;
using std::tr1::placeholders::_1;

#ifdef SIRIKATA_TRACK_EVENT_QUEUES
namespace {
typedef boost::mutex AllIOServicesMutex;
typedef boost::lock_guard<AllIOServicesMutex> AllIOServicesLockGuard;
AllIOServicesMutex gAllIOServicesMutex;
typedef std::tr1::unordered_set<IOService*> AllIOServicesSet;
AllIOServicesSet gAllIOServices;
} // namespace
#endif


IOService::IOService(const String& name)
 : mName(name)
#ifdef SIRIKATA_TRACK_EVENT_QUEUES
   ,
   mTimersEnqueued(0),
   mEnqueued(0),
   mWindowedTimerLatencyStats(100),
   mWindowedHandlerLatencyStats(100),
   mWindowedLatencyTagStats(40000)
#endif
{
    mImpl = new boost::asio::io_service(1);

#ifdef SIRIKATA_TRACK_EVENT_QUEUES
    AllIOServicesLockGuard lock(gAllIOServicesMutex);
    gAllIOServices.insert(this);
#endif
}

IOService::~IOService(){
    delete mImpl;

#ifdef SIRIKATA_TRACK_EVENT_QUEUES
    AllIOServicesLockGuard lock(gAllIOServicesMutex);
    gAllIOServices.erase(this);
#endif
}

IOStrand* IOService::createStrand(const String& name) {
    IOStrand* res = new IOStrand(*this, name);
#ifdef SIRIKATA_TRACK_EVENT_QUEUES
    LockGuard lock(mMutex);
    mStrands.insert(res);
#endif
    return res;
}

uint32 IOService::pollOne() {
    return (uint32) mImpl->poll_one();
}

uint32 IOService::poll() {
    return (uint32) mImpl->poll();
}

uint32 IOService::runOne() {
    return (uint32) mImpl->run_one();
}

uint32 IOService::run() {
    return (uint32) mImpl->run();
}

void IOService::runNoReturn() {
    mImpl->run();
}

void IOService::stop() {
    mImpl->stop();
}

void IOService::reset() {
    mImpl->reset();
}

void IOService::dispatch(
    const IOCallback& handler, const char* tag, const char* tagStat)
{
#ifdef SIRIKATA_TRACK_EVENT_QUEUES
    mEnqueued++;
    {
        LockGuard lock(mMutex);
        if (mTagCounts.find(tag) == mTagCounts.end())
            mTagCounts[tag] = 0;
        mTagCounts[tag]++;
    }
    mImpl->dispatch(
        std::tr1::bind(&IOService::decrementCount, this, Timer::now(), handler, tag,tagStat)
    );
#else
    mImpl->dispatch(handler);
#endif
}

void IOService::post(
    const IOCallback& handler, const char* tag, const char* tagStat)
{
#ifdef SIRIKATA_TRACK_EVENT_QUEUES
    mEnqueued++;
    {
        LockGuard lock(mMutex);
        if (mTagCounts.find(tag) == mTagCounts.end())
            mTagCounts[tag] = 0;
        mTagCounts[tag]++;
    }
    mImpl->post(
        std::tr1::bind(&IOService::decrementCount, this, Timer::now(), handler, tag,tagStat)
    );
#else
    mImpl->post(handler);
#endif
}

namespace {
void handle_deadline_timer(const boost::system::error_code&e, const deadline_timer_ptr& timer, const IOCallback& handler) {
    if (e)
        return;

    handler();
}
} // namespace

void IOService::post(const Duration& waitFor, const IOCallback& handler, const char* tag, const char* tagStat) {
#if BOOST_VERSION==103900
    static bool warnOnce=true;
    if (warnOnce) {
        warnOnce=false;
        SILOG(core,error,"Using buggy version of boost (1.39.0), leaking deadline_timer to avoid crash");
    }
#endif
    deadline_timer_ptr timer(new deadline_timer(*mImpl, posix_microseconds(waitFor.toMicroseconds())));

#ifdef SIRIKATA_TRACK_EVENT_QUEUES
    mTimersEnqueued++;
    {
        LockGuard lock(mMutex);
        if (mTagCounts.find(tag) == mTagCounts.end())
            mTagCounts[tag] = 0;
        mTagCounts[tag]++;
    }
    IOCallbackWithError orig_cb = std::tr1::bind(&handle_deadline_timer, _1, timer, handler);
    timer->async_wait(
        std::tr1::bind(&IOService::decrementTimerCount, this,
            _1, Timer::now(), waitFor, orig_cb, tag,tagStat
        )
    );
#else
    timer->async_wait(std::tr1::bind(&handle_deadline_timer, _1, timer, handler));
#endif
}



#ifdef SIRIKATA_TRACK_EVENT_QUEUES
void IOService::decrementTimerCount(const boost::system::error_code& e, const Time& start, const Duration& timer_duration, const IOCallbackWithError& cb, const char* tag, const char* tagStat) {
    mTimersEnqueued--;
    Time end = Timer::now();
    {
        LockGuard lock(mMutex);

        mWindowedTimerLatencyStats.sample((end - start) - timer_duration);


        assert(mTagCounts.find(tag) != mTagCounts.end());
        mTagCounts[tag]--;
    }

    Time begin = Timer::now();
    cb(e);
    end = Timer::now();

    TagDuration td;

    td.tag = tagStat == NULL ? tag : tagStat;
    td.dur = end-begin;
    {
        LockGuard lock(mMutex);
        mWindowedLatencyTagStats.sample(td);
    }
}

void IOService::decrementCount(const Time& start, const IOCallback& cb, const char* tag,const char* tagStat) {
    mEnqueued--;
    Time end = Timer::now();
    {
        LockGuard lock(mMutex);
        mWindowedHandlerLatencyStats.sample(end - start);
        assert(mTagCounts.find(tag) != mTagCounts.end());
        mTagCounts[tag]--;
    }


    Time begin = Timer::now();
    cb();
    end = Timer::now();

    TagDuration td;
    td.tag = tagStat == NULL ? tag : tagStat;
    td.dur = end-begin;
    {
        LockGuard lock(mMutex);
        mWindowedLatencyTagStats.sample(td);
    }
}


void IOService::destroyingStrand(IOStrand* child) {
    LockGuard lock(mMutex);
    assert(mStrands.find(child) != mStrands.end());
    mStrands.erase(child);
}

namespace {
// The original data
typedef std::tr1::unordered_map<const char*, uint32> TagCountMap;
// Reduced version, where different char*'s are converted to Strings so they
// will be aggregated
typedef std::tr1::unordered_map<String, uint32> ReducedTagCountMap;
// Inverted data, kept as a map so it's in sorted order
typedef std::multimap<uint32, String, std::greater<uint32> > InvertedReducedTagCountMap;

void getOffenders(const TagCountMap& orig_tags, InvertedReducedTagCountMap& tags_by_count_out) {
    // We need to reduce these in case we have 2 identical strings but they have
    // different char*'s.
    ReducedTagCountMap tags;
    for(TagCountMap::const_iterator it = orig_tags.begin(); it != orig_tags.end(); it++) {
        String key;
        if (it->first == NULL)
            key = "(NULL)";
        else
            key = it->first;
        if (tags.find(key) == tags.end()) tags[key] = 0;
        tags[key] += it->second;
    }

    // Invert and get sorted
    for(ReducedTagCountMap::iterator it = tags.begin(); it != tags.end(); it++)
        tags_by_count_out.insert(
            InvertedReducedTagCountMap::value_type(it->second, it->first)
        );
}

void reportOffenders(TagCountMap orig_tags) {
    InvertedReducedTagCountMap tags_by_count;
    getOffenders(orig_tags, tags_by_count);
    for(InvertedReducedTagCountMap::iterator it = tags_by_count.begin(); it != tags_by_count.end(); it++)
        if (it->first > 0) SILOG(ioservice, info, "      " << it->second << " (" << it->first << ")");
}

void reportOffenders(TagCountMap orig_tags, Command::Result& res_out, const String& path) {
    InvertedReducedTagCountMap tags_by_count;
    getOffenders(orig_tags, tags_by_count);

    res_out.put(path, Command::Array());
    Command::Array& items = res_out.getArray(path);

    for(InvertedReducedTagCountMap::iterator it = tags_by_count.begin(); it != tags_by_count.end(); it++) {
        if (it->first <= 0) continue;
        Command::Object tag_count;
        tag_count["tag"] = it->second;
        tag_count["count"] = it->first;
        items.push_back(tag_count);
    }
}

}

void IOService::reportStats() const {
    LockGuard lock(const_cast<Mutex&>(mMutex));

    SILOG(ioservice, info, "=======================================================");
    SILOG(ioservice, info, "'" << name() <<  "' IOService Statistics");
    SILOG(ioservice, info, "  Timers: " << numTimersEnqueued() << " with " << timerLatency() << " recent latency");
    SILOG(ioservice, info, "  Event handlers: " << numEnqueued() << " with " << handlerLatency() << " recent latency");
    reportOffenders(mTagCounts);

    for(StrandSet::const_iterator it = mStrands.begin(); it != mStrands.end(); it++) {
        SILOG(ioservice, info, "-------------------------------------------------------");
        SILOG(ioservice, info, "  Child '" << (*it)->name() << "'");
        SILOG(ioservice, info, "    Timers: " << (*it)->numTimersEnqueued() << " with " << (*it)->timerLatency() << " recent latency");
        SILOG(ioservice, info, "    Event handlers: " << (*it)->numEnqueued() << " with " << (*it)->handlerLatency() << " recent latency");
        reportOffenders((*it)->enqueuedTags());
    }
}

void IOService::reportStatsFile(const char* filename,bool detailed) const {
    LockGuard lock(const_cast<Mutex&>(mMutex));

    std::ofstream fileWriter(filename,std::ios::out|std::ios::app);
    fileWriter<<"=======================================================\n";
    fileWriter<< "'" << name() <<  "' IOService Statistics\n";
    fileWriter<<"  Timers: " << numTimersEnqueued() << " with " << timerLatency() << " recent latency\n";
    fileWriter<<"  Event handlers: " << numEnqueued() << " with " << handlerLatency() << " recent latency\n";

    for(StrandSet::const_iterator it = mStrands.begin();
        it != mStrands.end(); it++)
    {
        fileWriter<<"-------------------------------------------------------\n";
        fileWriter<<"  Child '" << (*it)->name() << "'\n";
        fileWriter<<"    Timers: " << (*it)->numTimersEnqueued() << " with " << (*it)->timerLatency() << " recent latency\n";
        fileWriter<<"    Event handlers: " << (*it)->numEnqueued() << " with " << (*it)->handlerLatency() << " recent latency\n";
    }

    if (detailed)
    {
        const CircularBuffer<TagDuration>& samples = mWindowedLatencyTagStats.getSamples();
        fileWriter<<"Sample size: "<<samples.size()<<"\n";
        for (std::size_t s = 0; s < samples.size(); ++s)
        {
            const TagDuration& data = samples[s];
            fileWriter<<" sample: "<< data.tag<<"  "<<data.dur<<"\n";
        }
    }

    fileWriter.flush();
    fileWriter.close();
}



void IOService::reportAllStats() {
    AllIOServicesLockGuard lock(gAllIOServicesMutex);
    for(AllIOServicesSet::const_iterator it = gAllIOServices.begin(); it != gAllIOServices.end(); it++)
        (*it)->reportStats();
}

void IOService::reportAllStatsFile(const char* filename,bool detailed) {
    AllIOServicesLockGuard lock(gAllIOServicesMutex);
    for(AllIOServicesSet::const_iterator it = gAllIOServices.begin(); it != gAllIOServices.end(); it++)
        (*it)->reportStatsFile(filename, detailed);
}


void IOService::fillCommandResultWithStats(Command::Result& res) {
    LockGuard lock(const_cast<Mutex&>(mMutex));

    res.put("name", name());
    res.put("timers.enqueued", numTimersEnqueued());
    res.put("timers.latency", timerLatency().toString());
    res.put("handlers.enqueued", numEnqueued());
    res.put("handlers.latency", handlerLatency().toString());
    reportOffenders(mTagCounts, res, "offenders");

    res.put("strands", Command::Array());
    Command::Array& strands = res.getArray("strands");
    for(StrandSet::const_iterator it = mStrands.begin(); it != mStrands.end(); it++) {
        strands.push_back(Command::Object());
        strands.back().put("name", (*it)->name());
        strands.back().put("timers.enqueued", (*it)->numTimersEnqueued());
        strands.back().put("timers.latency", (*it)->timerLatency().toString());
        strands.back().put("handlers.enqueued", (*it)->numEnqueued());
        strands.back().put("handlers.latency", (*it)->handlerLatency().toString());
        reportOffenders((*it)->enqueuedTags(), strands.back(), "offenders");
    }

}
#endif

void IOService::commandReportStats(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid) {
#ifdef SIRIKATA_TRACK_EVENT_QUEUES
    AllIOServicesLockGuard lock(gAllIOServicesMutex);

    Command::Result result = Command::EmptyResult();
    fillCommandResultWithStats(result);
    cmdr->result(cmdid, result);
#endif
}

void IOService::commandReportAllStats(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid) {
#ifdef SIRIKATA_TRACK_EVENT_QUEUES
    AllIOServicesLockGuard lock(gAllIOServicesMutex);

    Command::Result result = Command::EmptyResult();
    // Ensure the top-level structure is there
    result.put("ioservices", Command::Array());
    Command::Array& services = result.getArray("ioservices");

    for(AllIOServicesSet::const_iterator it = gAllIOServices.begin(); it != gAllIOServices.end(); it++) {
        services.push_back(Command::Object());
        (*it)->fillCommandResultWithStats(services.back());
    }
    cmdr->result(cmdid, result);
#endif
}



} // namespace Network
} // namespace Sirikata

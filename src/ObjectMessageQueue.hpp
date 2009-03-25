#ifndef _CBR_OBJECT_MESSAGE_QUEUE_HPP
#define _CBR_OBJECT_MESSAGE_QUEUE_HPP

#include "Utility.hpp"
#include "Statistics.hpp"
#include "LocationService.hpp"
#include "CoordinateSegmentation.hpp"

namespace CBR{
class ServerMessageQueue;
class ObjectToObjectMessage;

class ObjectMessageQueue {
public:
    ObjectMessageQueue(ServerMessageQueue*sm, LocationService* loc, CoordinateSegmentation* cseg, Trace* trace)
      : mServerMessageQueue(sm),
        mLocationService(loc),
        mCSeg(cseg),
        mTrace(trace)
    {}

    virtual ~ObjectMessageQueue(){}
    virtual bool send(ObjectToObjectMessage* msg) = 0;
    virtual void service(const Time& t)=0;

    virtual void registerClient(UUID oid,float weight=1) = 0;

protected:
    ServerID lookup(const UUID& obj_id) {
        Vector3f pos = mLocationService->currentPosition(obj_id);
        return mCSeg->lookup(pos);
    }

    ServerMessageQueue *mServerMessageQueue;
    LocationService* mLocationService;
    CoordinateSegmentation* mCSeg;
    Trace* mTrace;
};
}
#endif

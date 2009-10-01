#ifndef _CBR_ABSTRACTQUEUE_HPP_
#define _CBR_ABSTRACTQUEUE_HPP_

namespace CBR {

namespace QueueEnum {
    enum PushResult {
        PushSucceeded,
        PushExceededMaximumSize
    };
};

template <typename ElementType> class AbstractQueue {
public:
    typedef ElementType Type;
    typedef std::tr1::function<void(const ElementType&)> PopCallback;

    AbstractQueue(){
    }
    virtual ~AbstractQueue(){}

    virtual QueueEnum::PushResult push(const ElementType &msg)=0;

    virtual const ElementType& front() const=0;

    virtual ElementType& front()=0;

    virtual ElementType pop() { return ElementType(); }

    // XXX FIXME: This version of pop shouldn't go here, but is the stop
    // gap solution to getting layered Fair"Queue"s working together.
    // The callback parameter is a user provided callback which *must* be
    // called at a safe period when updating the queue, meaning that from
    // the perspective of the user.
    // The default implementation simply does a regular pop and calls the
    // callback, since in normal queues, the ordering isn't important.
    virtual ElementType pop(const PopCallback& cb) {
        ElementType popped = pop();
        if (cb != 0)
            cb(popped);
        return popped;
    }


    virtual bool empty() const=0;


/** FIXME this is unsafe since we need to track the size of all messages
    virtual std::deque<ElementType>& messages()=0;
*/

};

}
#endif

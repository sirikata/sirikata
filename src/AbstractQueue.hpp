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

    AbstractQueue(){
    }
    virtual ~AbstractQueue(){}

    virtual QueueEnum::PushResult push(const ElementType &msg)=0;

    virtual const ElementType& front() const=0;

    virtual ElementType& front()=0;

    virtual ElementType pop() { return ElementType(); }

    virtual bool empty() const=0;


/** FIXME this is unsafe since we need to track the size of all messages
    virtual std::deque<ElementType>& messages()=0;
*/

};

}
#endif

#ifndef _SIRIKATA_SIZE_ESTIMATOR_
#define _SIRIKATA_SIZE_ESTIMATOR_

namespace Sirikata {

class SIRIKATA_EXPORT SizeEstimator {
  public:
    virtual bool estimatedSizeReady() const = 0;
    virtual size_t getEstimatedSize() const = 0;
    virtual ~SizeEstimator(){}
};
class SIRIKATA_EXPORT ConstantSizeEstimator : public SizeEstimator {
    size_t mSize;
public:
    ConstantSizeEstimator(size_t size) : mSize(size) {}
    virtual bool estimatedSizeReady() const {
        return true;
    }
    virtual size_t getEstimatedSize() const {
        return mSize;
    }
};
}
#endif

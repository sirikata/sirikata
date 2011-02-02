
#ifndef __SIRIKATA_JS_WATCHABLE_HPP__
#define __SIRIKATA_JS_WATCHABLE_HPP__

#include <map>
#include "JSWhenStruct.hpp"



namespace Sirikata {
namespace JS {

class JSWhenStruct;

struct JSWatchable
{
    JSWatchable();
    virtual ~JSWatchable();
    virtual void setFlag();
    virtual void addWhen(JSWhenStruct* toAdd);
    virtual void checkAndClearFlag(WhenMap& whensToCheck);
    virtual void removeWhen(JSWhenStruct* toRemove);
    
private:
    virtual bool checkFlag();
    virtual void checkWhenConds(WhenMap&  whensToCheck);
    virtual void clearFlag();
    bool flag;
    WhenMap assocWhenConds;
};

typedef std::map<JSWatchable*, int> WatchableMap;
typedef WatchableMap::iterator WatchableIter;

} //end namespace js
} //end sirikata


#endif

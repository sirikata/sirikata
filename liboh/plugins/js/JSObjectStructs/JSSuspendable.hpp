
#ifndef __SIRIKATA_JS_SUSPENDABLE_HPP__
#define __SIRIKATA_JS_SUSPENDABLE_HPP__

#include <map>
#include <v8.h>



namespace Sirikata {
namespace JS {


struct JSSuspendable
{
    
    JSSuspendable();
    virtual ~JSSuspendable();
    virtual v8::Handle<v8::Value> suspend();
    virtual v8::Handle<v8::Value> resume();
    virtual v8::Handle<v8::Value> clear();

    //suspended boolean accessors
    v8::Handle<v8::Boolean> getIsSuspendedV8();
    bool getIsSuspended();

    //cleared boolean accessors
    v8::Handle<v8::Boolean> getIsClearedV8();
    bool getIsCleared();

    bool isSuspended;
    bool isCleared;
};

typedef std::map<JSSuspendable*, int> SuspendableMap;
typedef SuspendableMap::iterator SuspendableIter;

} //end namespace js
} //end sirikata


#endif

#include "JSWatchable.hpp"
#include "JSWhenStruct.hpp"

namespace Sirikata{
namespace JS{

JSWatchable::JSWatchable()
 : flag (false)
{}

JSWatchable::~JSWatchable()
{}

void JSWatchable::setFlag()
{
    flag = true;
}

bool JSWatchable::checkFlag()
{
    return flag;
}

void JSWatchable::clearFlag()
{
    flag = false;
}

void JSWatchable::removeWhen(JSWhenStruct* toRemove)
{
    WhenMapIter iter = assocWhenConds.find(toRemove);
    if (iter != assocWhenConds.end())
        assocWhenConds.erase(iter);
}

void JSWatchable::checkAndClearFlag(WhenMap& whensToCheck)
{
    checkWhenConds(whensToCheck);
    clearFlag();
}

void JSWatchable::addWhen(JSWhenStruct* toAdd)
{
    assocWhenConds[toAdd] = true;
}

void JSWatchable::checkWhenConds(WhenMap&  whensToCheck)
{
    if (flag)
    {
        for (WhenMapIter iter = assocWhenConds.begin(); iter != assocWhenConds.end(); ++iter)
            whensToCheck[iter->first] = true;
    }
}
    

} //end namespace js
} //end namespace sirikata

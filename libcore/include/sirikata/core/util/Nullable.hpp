#ifndef __NULLABLE_HPP__
#define __NULLABLE_HPP__

#include "Logging.hpp"

namespace Sirikata
{

template <typename nullable> class Nullable
{
public:
    Nullable(const nullable& nullableObject)
     : mNullable(nullableObject),
       internalIsNull(false)
    {
    }

    Nullable()
    {
        internalIsNull = true;
    }

    ~Nullable()
    {}

    /**
       Does not delete the internal object, but makes it so that it's
       inaccessible.
     */
    void makeNull()
    {
        internalIsNull = false;
    }

    /**
       Returns true if the internal object has not been set or has been cleared.
     */
    bool isNull()
    {
        return internalIsNull;
    }

    /**
       Issues warning if try to access a value when it's supposed to be null.
     */
    nullable getValue()
    {
        if (isNull())
            SILOG(util,error,"In Nullable, calling getValue on a null");
                
        return mNullable;
    }
    
    /**
       Set the internal object nullableObject
     */
    void setValue(const nullable& nullableObject)
    {
        internalIsNull = false;
        mNullable = nullableObject;
    }

    /**
       If pass no parameters to setValue, has same effect as makeNull.
     */
    void setValue()
    {
        makeNull();
    }
    
private:
    nullable mNullable;
    bool internalIsNull;

};




}//end namespace
#endif


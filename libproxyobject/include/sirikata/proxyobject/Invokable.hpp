#ifndef _SIRIKATA_INVOKABLE_HPP_
#define _SIRIKATA_INVOKABLE_HPP_

#include <sirikata/proxyobject/Platform.hpp>
#include <sirikata/core/util/SpaceObjectReference.hpp>
#include <vector>
#include <boost/any.hpp>


namespace Sirikata
{
  class SIRIKATA_PROXYOBJECT_EXPORT Invokable
  {
    public:
    virtual boost::any invoke(std::vector<boost::any>& params);
    boost::any invoke();
    virtual ~Invokable();

    typedef std::vector<boost::any> Array;
    typedef std::map<String, boost::any> Dict;

    // Because type_info's aren't guaranteed the same across shared libraries unless you
    // export *all* symbols, we include a bunch of utilties for performing
    // translations, which will all end up using *this* library's type_infos. As
    // long as you stick to using these utilities, everything should translate
    // back and forth to and from boost:any's in a way that all libraries can
    // safely interact with.  Note that this also means that you can only
    // *safely* use these types, but you shouldn't be using a wide variety of
    // types anyway since both ends need to know how to encode/decode the values.

    static bool anyIsBoolean(const boost::any& a);
    static bool anyAsBoolean(const boost::any& a);
    static bool anyIsFloat(const boost::any& a);
    static float32 anyAsFloat(const boost::any& a);
    static bool anyIsDouble(const boost::any& a);
    static float64 anyAsDouble(const boost::any& a);
    static bool anyIsNumeric(const boost::any& a);
    static float64 anyAsNumeric(const boost::any& a);

    static bool anyIsUInt8(const boost::any& a);
    static uint8 anyAsUInt8(const boost::any& a);
    static bool anyIsInt8(const boost::any& a);
    static int8 anyAsInt8(const boost::any& a);
    static bool anyIsUInt16(const boost::any& a);
    static uint16 anyAsUInt16(const boost::any& a);
    static bool anyIsInt16(const boost::any& a);
    static int16 anyAsInt16(const boost::any& a);
    static bool anyIsUInt32(const boost::any& a);
    static uint32 anyAsUInt32(const boost::any& a);
    static bool anyIsInt32(const boost::any& a);
    static int32 anyAsInt32(const boost::any& a);
    static bool anyIsUInt64(const boost::any& a);
    static uint64 anyAsUInt64(const boost::any& a);
    static bool anyIsInt64(const boost::any& a);
    static int64 anyAsInt64(const boost::any& a);

    static bool anyIsString(const boost::any& a);
    static String anyAsString(const boost::any& a);
    static bool anyIsInvokable(const boost::any& a);
    static Invokable* anyAsInvokable(const boost::any& a);
    static bool anyIsObject(const boost::any& a);
    static SpaceObjectReference anyAsObject(const boost::any& a);
    static bool anyIsDict(const boost::any& a);
    static Dict anyAsDict(const boost::any& a);
    static bool anyIsArray(const boost::any& a);
    static Array anyAsArray(const boost::any& a);

    // Note that these are specifically *not* templated so they are forced to be
    // compiled into this library, guaranteeing proper type_info's.
    static boost::any asAny(bool b);
    static boost::any asAny(uint8 b);
    static boost::any asAny(int8 b);
    static boost::any asAny(uint16 b);
    static boost::any asAny(int16 b);
    static boost::any asAny(uint32 b);
    static boost::any asAny(int32 b);
    static boost::any asAny(uint64 b);
    static boost::any asAny(int64 b);
    static boost::any asAny(float b);
    static boost::any asAny(double b);
    static boost::any asAny(const String& b);
    static boost::any asAny(Invokable* b);
    static boost::any asAny(const SpaceObjectReference& b);
    static boost::any asAny(const Dict& b);
    static boost::any asAny(const Array& b);
  };
}

#endif

#include <sirikata/proxyobject/Invokable.hpp>

namespace Sirikata
{

boost::any Invokable::invoke(std::vector<boost::any>& params) {
    return boost::any();
}

Invokable::~Invokable() {
}

bool Invokable::anyIsBoolean(const boost::any& a) {
    return (!a.empty() && a.type() == typeid(bool));
}

bool Invokable::anyAsBoolean(const boost::any& a) {
    return boost::any_cast<bool>(a);
}

bool Invokable::anyIsFloat(const boost::any& a) {
    return (!a.empty() && a.type() == typeid(float32));
}

float32 Invokable::anyAsFloat(const boost::any& a) {
    return boost::any_cast<float32>(a);
}

bool Invokable::anyIsDouble(const boost::any& a) {
    return (!a.empty() && a.type() == typeid(float64));
}

float64 Invokable::anyAsDouble(const boost::any& a) {
    return boost::any_cast<float64>(a);
}

bool Invokable::anyIsNumeric(const boost::any& a) {
    return anyIsFloat(a) || anyIsDouble(a);
}

float64 Invokable::anyAsNumeric(const boost::any& a) {
    if (anyIsFloat(a)) return anyAsFloat(a);
    else return anyAsDouble(a);
}


bool Invokable::anyIsUInt8(const boost::any& a) {
    return (!a.empty() && a.type() == typeid(float64));
}

uint8 Invokable::anyAsUInt8(const boost::any& a) {
    return boost::any_cast<uint8>(a);
}

bool Invokable::anyIsInt8(const boost::any& a) {
    return (!a.empty() && a.type() == typeid(float64));
}

int8 Invokable::anyAsInt8(const boost::any& a) {
    return boost::any_cast<int8>(a);
}


bool Invokable::anyIsUInt16(const boost::any& a) {
    return (!a.empty() && a.type() == typeid(float64));
}

uint16 Invokable::anyAsUInt16(const boost::any& a) {
    return boost::any_cast<uint16>(a);
}

bool Invokable::anyIsInt16(const boost::any& a) {
    return (!a.empty() && a.type() == typeid(float64));
}

int16 Invokable::anyAsInt16(const boost::any& a) {
    return boost::any_cast<int16>(a);
}


bool Invokable::anyIsUInt32(const boost::any& a) {
    return (!a.empty() && a.type() == typeid(float64));
}

uint32 Invokable::anyAsUInt32(const boost::any& a) {
    return boost::any_cast<uint32>(a);
}

bool Invokable::anyIsInt32(const boost::any& a) {
    return (!a.empty() && a.type() == typeid(float64));
}

int32 Invokable::anyAsInt32(const boost::any& a) {
    return boost::any_cast<int32>(a);
}


bool Invokable::anyIsUInt64(const boost::any& a) {
    return (!a.empty() && a.type() == typeid(float64));
}

uint64 Invokable::anyAsUInt64(const boost::any& a) {
    return boost::any_cast<uint64>(a);
}

bool Invokable::anyIsInt64(const boost::any& a) {
    return (!a.empty() && a.type() == typeid(float64));
}

int64 Invokable::anyAsInt64(const boost::any& a) {
    return boost::any_cast<int64>(a);
}


bool Invokable::anyIsString(const boost::any& a) {
    return (!a.empty() && a.type() == typeid(String));
}

String Invokable::anyAsString(const boost::any& a) {
    return boost::any_cast<String>(a);
}

bool Invokable::anyIsInvokable(const boost::any& a) {
    return (!a.empty() && a.type() == typeid(Invokable*));
}

Invokable* Invokable::anyAsInvokable(const boost::any& a) {
    return boost::any_cast<Invokable*>(a);
}

bool Invokable::anyIsObject(const boost::any& a) {
    return (!a.empty() && a.type() == typeid(SpaceObjectReference));
}

SpaceObjectReference Invokable::anyAsObject(const boost::any& a) {
    return boost::any_cast<SpaceObjectReference>(a);
}

bool Invokable::anyIsDict(const boost::any& a) {
    return (!a.empty() && a.type() == typeid(Dict));
}

Invokable::Dict Invokable::anyAsDict(const boost::any& a) {
    return boost::any_cast<Dict>(a);
}

bool Invokable::anyIsArray(const boost::any& a) {
    return (!a.empty() && a.type() == typeid(Array));
}

Invokable::Array Invokable::anyAsArray(const boost::any& a) {
    return boost::any_cast<Array>(a);
}



boost::any Invokable::asAny(bool b) {
    return boost::any(b);
}

boost::any Invokable::asAny(uint8 b) {
    return boost::any(b);
}

boost::any Invokable::asAny(int8 b) {
    return boost::any(b);
}

boost::any Invokable::asAny(uint16 b) {
    return boost::any(b);
}

boost::any Invokable::asAny(int16 b) {
    return boost::any(b);
}

boost::any Invokable::asAny(uint32 b) {
    return boost::any(b);
}

boost::any Invokable::asAny(int32 b) {
    return boost::any(b);
}

boost::any Invokable::asAny(uint64 b) {
    return boost::any(b);
}

boost::any Invokable::asAny(int64 b) {
    return boost::any(b);
}


boost::any Invokable::asAny(float b) {
    return boost::any(b);
}

boost::any Invokable::asAny(double b) {
    return boost::any(b);
}

boost::any Invokable::asAny(const String& b) {
    return boost::any(b);
}

boost::any Invokable::asAny(Invokable* b) {
    return boost::any(b);
}

boost::any Invokable::asAny(const SpaceObjectReference& b) {
    return boost::any(b);
}

boost::any Invokable::asAny(const Dict& b) {
    return boost::any(b);
}

boost::any Invokable::asAny(const Array& b) {
    return boost::any(b);
}

}

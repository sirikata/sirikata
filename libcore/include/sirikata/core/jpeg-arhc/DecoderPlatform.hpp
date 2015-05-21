#ifdef __linux
#define SIRIKATA_FUNCTION_EXPORT __attribute__ ((visibility("default")))
#define SIRIKATA_EXPORT __attribute__ ((visibility("default")))
#define SIRIKATA_PLUGIN_EXPORT __attribute__ ((visibility("default")))
#else
#include <sirikata/core/util/Platform.hpp>
#endif

#include <stdint.h>
#include <stddef.h>
#include <string>
#include <vector>
namespace Sirikata{

typedef int64_t int64;
typedef uint64_t uint64;
typedef int32_t int32;
typedef uint32_t uint32;
typedef uint16_t uint16;
typedef int16_t int16;
typedef uint8_t uint8;
typedef uint8_t byte;
typedef int8_t int8;

}

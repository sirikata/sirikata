namespace Sirikata {
namespace Logging {
enum LOGGING_LEVEL {
    fatal=1,
    error=8,
    warning=64,
    info=512,
    debug=4096,
    insane=32768
};

extern OptionValue* defaultLevel;
extern OptionValue* atLeastLevel;
extern OptionValue* moduleLevel;
} }
#if 1
#define SILOGP(module,lvl) \
    (Sirikata::Logging::defaultLevel->as<Sirikata::Logging::LOGGING_LEVEL>()>=Sirikata::Logging::lvl&& \
        ( (Sirikata::Logging::moduleLevel->as<std::tr1::unordered_map<std::string,Sirikata::Logging::LOGGING_LEVEL> >().find(#module)==Sirikata::Logging::moduleLevel->as<std::tr1::unordered_map<std::string,Sirikata::Logging::LOGGING_LEVEL> >().end() && \
           Sirikata::Logging::defaultLevel->as<Sirikata::Logging::LOGGING_LEVEL>()>=(Sirikata::Logging::lvl)) \
          || (Sirikata::Logging::moduleLevel->as<std::tr1::unordered_map<std::string,Sirikata::Logging::LOGGING_LEVEL> >().find(#module)!=Sirikata::Logging::moduleLevel->as<std::tr1::unordered_map<std::string,Sirikata::Logging::LOGGING_LEVEL> >().end() && \
              Sirikata::Logging::moduleLevel->as<std::tr1::unordered_map<std::string,Sirikata::Logging::LOGGING_LEVEL> >()[#module]>=Sirikata::Logging::lvl)))
#define SILOGNOCR(module,lvl,value) \
    if (SILOGP(module,lvl)) \
        std::cerr << value
#define SILOG(module,lvl,value) SILOGNOCR(module,lvl,value) << std::endl
#else
#define SILOGP(module,lvl) false
#define SILOGNOCR(module,lvl,value)
#define SILOG(module,lvl,value)
#endif

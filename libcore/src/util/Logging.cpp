#include "Standard.hh"
namespace Sirikata { namespace Logging {
class LogLevelParser {public:
    static LOGGING_LEVEL lex_cast(const std::string&value) {
        if (value=="warning")
            return warning;
        if (value=="info")
            return info;
        if (value=="error")
            return error;
        if (value=="fatal")
            return fatal;
        if (value=="debug")
            return debug;
        return insane;
    }
    Any operator()(const std::string&value) {
        return lex_cast(value);
    }
};
class LogLevelMapParser {public:
    Any operator()(const std::string&cvalue) {
        std::tr1::unordered_map<std::string,LOGGING_LEVEL> retval;
        std::string value=cvalue;
        while (true) {
            std::string::size_type where=value.find_first_of(",");
            std::string comma;
            if (where!=std::string::npos) {
                comma=value.substr(0,where);
                value=value.substr(where+1);
            }else {
                comma=value;
            }
            std::string::size_type whereequal=comma.find_first_of("=");
            if (whereequal!=std::string::npos) {
                retval[comma.substr(0,whereequal)]=LogLevelParser::lex_cast(comma.substr(whereequal+1));
            }
            if (where==std::string::npos) {
                break;
            }
        }
        return retval;
    }
};

OptionValue* defaultLevel;
OptionValue* atLeastLevel;
OptionValue* moduleLevel;
InitializeGlobalOptions o("",
                    defaultLevel=new OptionValue("loglevel",
#ifdef NDEBUG
                                                 "info"
#else
                                                 "debug",
#endif
                                                 "Sets the default level for logging when no per-module override",
                                                 LogLevelParser()),
                    atLeastLevel=new OptionValue("maxloglevel",
#ifdef NDEBUG
                                                 "info"
#else
                                                 "insane"
#endif
                                                 ,"Sets the maximum logging level any module may be set to",
                                                 LogLevelParser()),
                    moduleLevel=new OptionValue("moduleloglevel",
                                                "",
                                                "Sets a per-module logging level: should be formatted <module>=debug,<othermodule>=info...",
                                                LogLevelMapParser()),
                     NULL);
                    
std::tr1::unordered_map<std::string,LOGGING_LEVEL> module_level;

} }

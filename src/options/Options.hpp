#include "util/Any.hpp"
namespace Iridium {
class OptionSet;
/**
 * A class that holds a particular option value, readable by other parts of the program as well as settable by those parts
 */
class OptionValue{
    Any mValue;
    std::string mDefaultValue;
    boost::function1<Any,std::string> mParser;
    boost::function3<void,const string&,Any,Any>mChangeFunction;
    std::string mName;
    template <class T> class Type {
        static Any lexical_cast(const std::string &value){
            T retval;
            std::istringstream ss(value);
            ss>>retval;
            return retval;
        }
    };
    template <> class Type<std::string> {
        static Any lexical_cast(const std::string &value){
        return value;
        }
    }
    
    static void noop(const string&,Any ,Any) {
        
    }
    friend class OptionSet;
    /**
     * Initializes an OptionValue and asserts that the mParser is NULL (in case 2 people added the same option)
     */
    OptionValue& initializationSet(const OptionValue&other);
public:
    ///changes the option value and invokes mChangeFunction message
    OptionValue& operator=(const OptionValue&other);
    
    OptionValue() {
        
    }
    /**
     * Invoke an option, setting its default value with a particular lexiccal cast function, a description and optionally a pointer to set to the result
     * \parameter option is the name of the option in the config or text files
     * \parameter defaultValue is the string representation of the default value as if a user typed it in
     * \parameter type is a class that contains a static lexical_cast function that transforms a string into an Any of the appropirate type
     * \parameter description is the textual description for the user when looking through the command line help
     * \parameter pointer holds a pointer to an OptionValue that will get set to the newly constructed class
     */
    template<class T>OptionValue(const std::string &option, const std::string&defaultValue, T type, const std::string&description, OptionValue**pointer){
        mParser=boost::function1<Any,std::string>(&T::lexical_cast);
        mName=option;
        mDefaultValue=defaultValue;        
        mDescription=description;
        mChangeFunction=boost::function3<void,const string&, Any, Any>(&OptionValue::noop);
        if (pointer)
            *pointer=this;
    }
    /**
     * Invoke an option, setting its default value with a particular lexiccal cast function, a description and optionally a pointer to set to the result
     * \parameter option is the name of the option in the config or text files
     * \parameter defaultValue is the string representation of the default value as if a user typed it in
     * \parameter type is a class that contains a static lexical_cast function that transforms a string into an Any of the appropirate type
     * \parameter description is the textual description for the user when looking through the command line help
     * \parameter changeFunction is the function that will be invoked if someone atomically changes the OptionValue type by invoking =
     * \parameter pointer holds a pointer to an OptionValue that will get set to the newly constructed class
     */
    template<class T>OptionValue(const std::string &option, const std::string&defaultValue, T type, const std::string&description, boost::function3<void,const string&,Any,Any>&changeFunction, OptionValue**pointer=NULL) {
        mParser=boost::function1<Any,std::string>(&T::lexical_cast);
        mName=option;
        mDescription=description;
        mDefaultValue=defaultValue;
        mChangeFunction=boost::function3<void,const string&, Any, Any>(&OptionValue::noop);
        if (pointer)
            *pointer=this;
    }
    /**
     * Invoke an option, setting its default value with a particular lexiccal cast function, a description and optionally a pointer to set to the result
     * \parameter option is the name of the option in the config or text files
     * \parameter defaultValue is the string representation of the default value as if a user typed it in
     * \parameter type is a class that contains a static lexical_cast function that transforms a string into an Any of the appropirate type
     * \parameter description is the textual description for the user when looking through the command line help
     * \parameter parser is the function that can convert a string to the boost::any of the appropriate type
     * \parameter pointer holds a pointer to an OptionValue that will get set to the newly constructed class
     */
    OptionValue(const std::string &option, const std::string&defaultValue, const std::string&description, const boost::function1<Any,std::string>& parser, OptionValue**pointer=NULL) {
        mParser=parser;
        mName=option;
        mDescription=description;
        mDefaultValue=defaultValue;
        mChangeFunction=changeFunction;
        if (pointer)
            *pointer=this;
    }
    /**
     * Invoke an option, setting its default value with a particular lexiccal cast function, a description and optionally a pointer to set to the result
     * \parameter option is the name of the option in the config or text files
     * \parameter defaultValue is the string representation of the default value as if a user typed it in
     * \parameter type is a class that contains a static lexical_cast function that transforms a string into an Any of the appropirate type
     * \parameter description is the textual description for the user when looking through the command line help
     * \parameter parser is the function that can convert a string to the boost::any of the appropriate type
     * \parameter changeFunction is the function that will be invoked if someone atomically changes the OptionValue type by invoking =
     * \parameter pointer holds a pointer to an OptionValue that will get set to the newly constructed class
     */
    OptionValue(const std::string &option, const std::string&defaultValue, const std::string&description, const boost::function1<Any,std::string>& parser,  const boost::function3<void,const string&, Any, Any> &changeFunction, OptionValue**pointer=NULL) {
        mParser=parser;
        mName=option;
        mDescription=description;
        mDefaultValue=defaultValue;
        mChangeFunction=changeFunction;
        if (pointer)
            *pointer=this;
    }
};
/**
 * This class holds a set of options that may appear on a command line or within an argument to a module
 * Holds a static index to all OptionSets currently available in the program.
 */
class OptionSet { 
    std::map<std::string,OptionValue*> mNames;

public:
    
    void parse(const std::string&);
    void parse(int, const char *);
    OptionValue* referenceOption(const std::string &option, OptionValue**pointer);
    static std::map<std::string,OptionSet*>* optionSets() {
        static std::map<std::string,OptionSet*>*retval=new std::map<std::string,OptionSet*>();
        return retval;
    }
};
/**
 * A dummy class to statically initialize a bunch of option classes that could add to a module
 */
class InitializeOptions {
public:
    ///Takes a null terminated list of OptionValues that should be added to the option set
    InitializeOptions(const char *,OptionValue*,...);
};
}

/*example options
extern OptionValue * SHIELD_ENERGY=NULL;
static InitializeOptions o("",
                           addOption("ShieldOption",2,"Sets the awesome option",&SHIELD_ENERGY),
                           addOption("server",IPAddress(127,0,0,1,24)),
                           addOption("client",IPAddress(127,0,0,1,24)),
                           addOption("server",IPAddress(127,0,0,1,24)),
                           addOption("server",IPAddress(127,0,0,1,24))
                           NULL);
extern OptionValue SHIELD_OPTION = referenceOption(CURRENT_MODULE,"ShieldOption");
*/

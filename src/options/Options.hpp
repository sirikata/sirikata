/*     Iridium Configuration Options -- Iridium Options
 *  Options.hpp
 *
 *  Copyright (c) 2009, Daniel Reiter Horn
 *  All rights reserved.
 * 
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Iridium nor the names of its contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "util/Any.hpp"
#include <string>
#include <boost/function.hpp>
#include <map>
namespace Iridium {
class OptionSet;
template <class T> class OptionValueType {
    static Any lexical_cast(const std::string &value){
        T retval;
        std::istringstream ss(value);
        ss>>retval;
        return retval;
    }
};
template <> class OptionValueType<std::string> {
    static Any lexical_cast(const std::string &value){
        return value;
    }
};
class OptionRegistration;

/**
 * A class that holds a particular option value, readable by other parts of the program as well as settable by those parts
 */
class OptionValue{
    Any mValue;
    std::string mDefaultValue;
    const char* mDescription;
    boost::function1<Any,std::string> mParser;
    boost::function3<void,const std::string&,Any,Any>mChangeFunction;
    const char* mName;
    
    static void noop(const std::string&,Any ,Any) {
        
    }
    ///Option Registration needs to have access to mValue to set it for the very first time without triggering event
    friend class OptionRegistration;
    ///OptionSet needs to have access to the members directly to return them
    friend class OptionSet;
    /**
     * Initializes an OptionValue and asserts that the mParser is NULL (in case 2 people added the same option)
     * \returns true unless there was a conflict in setting the variable
     */
    bool initializationSet(const OptionValue&other);
public:
    const char*description() const{
        return mDescription;
    }
    const std::string&defaultValue() const{
        return mDefaultValue;
    }
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
    template<class T>OptionValue(const char*option, const std::string&defaultValue, T type, const char*description, OptionValue**pointer){
        mParser=boost::function1<Any,std::string>(&T::lexical_cast);
        mName=option;
        mDefaultValue=defaultValue;        
        mDescription=description;
        mChangeFunction=boost::function3<void,const std::string&, Any, Any>(&OptionValue::noop);
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
    template<class T>OptionValue(const char* option, const std::string&defaultValue, T xtype, const char*description, boost::function3<void,const std::string&,Any,Any>&changeFunction, OptionValue**pointer=NULL) {
        mParser=boost::function1<Any,std::string>(&T::lexical_cast);
        mName=option;
        mDescription=description;
        mDefaultValue=defaultValue;
        mChangeFunction=boost::function3<void,const std::string&, Any, Any>(&OptionValue::noop);
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
    OptionValue(const char* option, const std::string&defaultValue, const char *description, const boost::function1<Any,std::string>& parser, OptionValue**pointer=NULL) {
        mParser=parser;
        mName=option;
        mDescription=description;
        mDefaultValue=defaultValue;
        mChangeFunction=&OptionValue::noop;
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
    OptionValue(const char* option, const std::string&defaultValue, const char* description, const boost::function1<Any,std::string>& parser,  const boost::function3<void,const std::string&, Any, Any> &changeFunction, OptionValue**pointer=NULL) {
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
 * A dummy class to statically initialize a bunch of option classes that could add to a module
 */
class InitializeOptions {
public:
    ///Takes a null terminated arg tuple of OptionValues* that should be added to the option set
    InitializeOptions(const char *,...);
};
/**
 * This class holds a set of options that may appear on a command line or within an argument to a module
 * Holds a static index to all OptionSets currently available in the program.
 */
class OptionSet { 
    std::map<std::string,OptionValue*> mNames;
    friend class InitializeOptions;
    void addOptionNoLock(OptionValue*);
public:
    
    void parse(const std::string&);
    void parse(int, const char **);
    void addOption(OptionValue*v);
    OptionValue* referenceOption(const std::string &option, OptionValue**pointer);
    static std::map<std::string,OptionSet*>* optionSets() {
        static std::map<std::string,OptionSet*>*retval=new std::map<std::string,OptionSet*>();
        return retval;
    }
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

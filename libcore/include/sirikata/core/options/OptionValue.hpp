/*  Sirikata Configuration Options -- Sirikata Options
 *  OptionValue.hpp
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
 *  * Neither the name of Sirikata nor the names of its contributors may
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

#ifndef _SIRIKATA_OPTIONVALUE_HPP_
#define _SIRIKATA_OPTIONVALUE_HPP_

#include <sirikata/core/util/Any.hpp>

namespace Sirikata {
class OptionSet;

// Strings

template <class T> class OptionValueType {public:
    static Any lexical_cast(const std::string &value){
        T retval=T();
        std::istringstream ss(value);
        ss>>retval;
        return retval;
    }
};
template <> class OptionValueType<std::string> {public:
    static Any lexical_cast(const std::string &value){
        return value;
    }
};

// Maps

template <class T> class OptionValueMap {public:
    /// makes a map out of an option like  "a:{b},c:{d}" mapping a->b c->d
    static Any lexical_cast(const std::string &value){
        T retval;
        std::string::size_type where=0,oldwhere=0;
        while((where=value.find(':',oldwhere))!=std::string::npos) {
            std::string key=value.substr(oldwhere,where-oldwhere);
            oldwhere=where+1;
            where=value.find('{',oldwhere);
            if (where!=std::string::npos) {
                int count=1;
                std::string::size_type value_begin=oldwhere=where+1;
                while ((where=value.find_first_of("{}",oldwhere))!=std::string::npos) {
                    if (value[where]=='}')
                        --count;
                    else
                        ++count;
                    oldwhere=where+1;
                    if (count==0){
                        std::string val=value.substr(value_begin,where-value_begin);
                        retval[key]=val;
                        break;
                    }
                }
            }
            where=value.find(',',oldwhere);
            if (where==std::string::npos) break;
            oldwhere=where+1;
        }
        return retval;
    }
};

template <> class OptionValueType<std::map<std::string,std::string> > :public OptionValueMap<std::map<std::string,std::string> > {public:
};
template <> class OptionValueType<std::tr1::unordered_map<std::string,std::string> > :public OptionValueMap<std::tr1::unordered_map<std::string,std::string> > {public:
};

// Lists

template <class T>
class OptionValueList {
public:
    /// Makes a list [a, b, c, d] out of an option of the form "a,b,c,d" or "[a,b,c,d]"
    static Any lexical_cast(const std::string &value){
        T retval;

        // Strip any [] around the list
        int32 list_start = 0, list_end = value.size();
        while(list_start < value.size()) {
            if (value[list_start] == '[') {
                list_start++;
                break;
            }
            else if (value[list_start] == ' ' || value[list_start] == '\t')
                list_start++;
            else
                break;
        }
        while(list_end >= 0) {
            if (value[list_end-1] == ']') {
                list_end--;
                break;
            }
            else if (value[list_end-1] == ' ' || value[list_end-1] == '\t')
                list_end--;
            else
                break;
        }

        if (list_end - list_start <= 0) return retval;

        int32 comma = list_start, last_comma = list_start-1;

        while(true) {
            comma = (int32)value.find(',', last_comma+1);
            if (comma > list_end || comma == std::string::npos)
                comma = list_end;
            std::string elem = value.substr(last_comma+1, (comma-(last_comma+1)));
            if (elem.size() > 0)
                retval.push_back(elem);

            // If we hit the end of the string, finish up
            if (comma >= list_end)
                break;

            last_comma = comma;
        }
        return retval;
    }
};

template <> class OptionValueType<std::vector<std::string> > :public OptionValueList<std::vector<std::string> > {public:
};
template <> class OptionValueType<std::list<std::string> > :public OptionValueList<std::list<std::string> > {public:
};

// Bool

template <> class OptionValueType<bool> {public:
    static Any lexical_cast(const std::string &value){
        bool retval=false;
        if (value.size()){
            if (value[0]=='T'||value[0]=='t'||value[0]=='1'||value[0]=='Y'||value[0]=='y')
                retval=true;
        }
        return retval;
    }
};
class OptionRegistration;

/**
 * A class that holds a particular option value, readable by other parts of the program as well as settable by those parts
 */
class OptionValue{
    Any mValue;
    std::string mDefaultValue;
    const char *mDefaultChar;
    const char* mDescription;
    std::tr1::function<Any(std::string)> mParser;
    std::tr1::function<void(const std::string&,Any,Any)>mChangeFunction;
    const char* mName;

    static void noop(const std::string&,Any ,Any) {

    }
    ///Option Registration needs to have access to mValue to set it for the very first time without triggering event
    friend class OptionRegistration;
    ///OptionSet needs to have access to the members directly to return them
    friend class OptionSet;

public:

    const Any*operator->()const {
        return &mValue;
    }
    Any*operator->() {
        return &mValue;
    }
    const Any*get()const {
        return &mValue;
    }
    Any* get() {
        return &mValue;
    }
    template <class T> T&as(){return mValue.as<T>();}
    template <class T> const T&as()const{return mValue.as<T>();}
    template <class T> T&unsafeAs(){return mValue.unsafeAs<T>();}
    template <class T> const T&unsafeAs()const{return mValue.unsafeAs<T>();}
    const char*description() const{
        return mDescription;
    }
    const char *defaultValue() const{
        return mDefaultChar?mDefaultChar:mDefaultValue.c_str();
    }
    ///changes the option value and invokes mChangeFunction message
    OptionValue& operator=(const OptionValue&other);

    OptionValue() {
        mDefaultChar=NULL;
        mDescription="";mName="";
        mChangeFunction=NULL;
        mParser=NULL;
    }
    /**
     * Invoke an option, setting its default value with a particular lexiccal cast function, a description and optionally a pointer to set to the result
     * \param option is the name of the option in the config or text files
     * \param defaultValue is the string representation of the default value as if a user typed it in
     * \param type is a class that contains a static lexical_cast function that transforms a string into an Sirikata::Any of the appropirate type
     * \param description is the textual description for the user when looking through the command line help
     * \param pointer holds a pointer to an OptionValue that will get set to the newly constructed class
     */
    template<class T>OptionValue(const char*option, const std::string&defaultValue, T type, const char*description, OptionValue**pointer=NULL):mDefaultChar(NULL){
        mParser=std::tr1::function<Any(std::string)>(&T::lexical_cast);
        mName=option;
        mDefaultValue=defaultValue;
        mDescription=description;
        mChangeFunction=std::tr1::function<void(const std::string&, Any, Any)>(&OptionValue::noop);
        if (pointer)
            *pointer=this;
    }
    /**
     * Invoke an option, setting its default value with a particular lexiccal cast function, a description and optionally a pointer to set to the result
     * \param option is the name of the option in the config or text files
     * \param defaultValue is the string representation of the default value as if a user typed it in
     * \param xtype is a class that contains a static lexical_cast function that transforms a string into an Sirikata::Any of the appropirate type
     * \param description is the textual description for the user when looking through the command line help
     * \param changeFunction is the function that will be invoked if someone atomically changes the OptionValue type by invoking =
     * \param pointer holds a pointer to an OptionValue that will get set to the newly constructed class
     */
    template<class T>OptionValue(const char* option, const std::string&defaultValue, T xtype, const char*description, std::tr1::function<void(const std::string&,Any,Any)>&changeFunction, OptionValue**pointer=NULL) :mDefaultChar(NULL){
        mParser=std::tr1::function<Any(std::string)>(&T::lexical_cast);
        mName=option;
        mDescription=description;
        mDefaultValue=defaultValue;
        mChangeFunction=std::tr1::function<void(const std::string&, Any, Any)>(&OptionValue::noop);
        if (pointer)
            *pointer=this;
    }
    /**
     * Invoke an option, setting its default value with a particular lexiccal cast function, a description and optionally a pointer to set to the result
     * \param option is the name of the option in the config or text files
     * \param defaultValue is the string representation of the default value as if a user typed it in
     * \param description is the textual description for the user when looking through the command line help
     * \param parser is the function that can convert a string to the Sirikata::Any of the appropriate type
     * \param pointer holds a pointer to an OptionValue that will get set to the newly constructed class
     */
    OptionValue(const char* option, const std::string&defaultValue, const char *description, const std::tr1::function<Any(std::string)>& parser, OptionValue**pointer=NULL) :mDefaultChar(NULL){
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
     * \param option is the name of the option in the config or text files
     * \param defaultValue is the string representation of the default value as if a user typed it in
     * \param description is the textual description for the user when looking through the command line help
     * \param parser is the function that can convert a string to the Sirikata::Any of the appropriate type
     * \param changeFunction is the function that will be invoked if someone atomically changes the OptionValue type by invoking =
     * \param pointer holds a pointer to an OptionValue that will get set to the newly constructed class
     */
    OptionValue(const char* option, const std::string&defaultValue, const char* description, const std::tr1::function<Any(std::string)>& parser,  const std::tr1::function<void(const std::string&, Any, Any)> &changeFunction, OptionValue**pointer=NULL) :mDefaultChar(NULL){
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
     * \param option is the name of the option in the config or text files
     * \param defaultValue is the string representation of the default value as if a user typed it in
     * \param type is a class that contains a static lexical_cast function that transforms a string into an Sirikata::Any of the appropirate type
     * \param description is the textual description for the user when looking through the command line help
     * \param pointer holds a pointer to an OptionValue that will get set to the newly constructed class
     */
    template<class T>OptionValue(const char*option, const char*defaultValue, T type, const char*description, OptionValue**pointer=NULL){
        mParser=std::tr1::function<Any(std::string)>(&T::lexical_cast);
        mName=option;
        mDefaultChar=defaultValue;
        mDescription=description;
        mChangeFunction=std::tr1::function<void(const std::string&, Any, Any)>(&OptionValue::noop);
        if (pointer)
            *pointer=this;
    }
    /**
     * Invoke an option, setting its default value with a particular lexiccal cast function, a description and optionally a pointer to set to the result
     * \param option is the name of the option in the config or text files
     * \param defaultValue is the string representation of the default value as if a user typed it in
     * \param xtype is a class that contains a static lexical_cast function that transforms a string into an Sirikata::Any of the appropirate type
     * \param description is the textual description for the user when looking through the command line help
     * \param changeFunction is the function that will be invoked if someone atomically changes the OptionValue type by invoking =
     * \param pointer holds a pointer to an OptionValue that will get set to the newly constructed class
     */
    template<class T>OptionValue(const char* option, const char*defaultValue, T xtype, const char*description, std::tr1::function<void(const std::string&,Any,Any)>&changeFunction, OptionValue**pointer=NULL) {
        mParser=std::tr1::function<Any(std::string)>(&T::lexical_cast);
        mName=option;
        mDescription=description;
        mDefaultChar=defaultValue;
        mChangeFunction=std::tr1::function<void(const std::string&, Any, Any)>(&OptionValue::noop);
        if (pointer)
            *pointer=this;
    }
    /**
     * Invoke an option, setting its default value with a particular lexiccal cast function, a description and optionally a pointer to set to the result
     * \param option is the name of the option in the config or text files
     * \param defaultValue is the string representation of the default value as if a user typed it in
     * \param description is the textual description for the user when looking through the command line help
     * \param parser is the function that can convert a string to the Sirikata::Any of the appropriate type
     * \param pointer holds a pointer to an OptionValue that will get set to the newly constructed class
     */
    OptionValue(const char* option, const char*defaultValue, const char *description, const std::tr1::function<Any(std::string)>& parser, OptionValue**pointer=NULL) {
        mParser=parser;
        mName=option;
        mDescription=description;
        mDefaultChar=defaultValue;
        mChangeFunction=&OptionValue::noop;
        if (pointer)
            *pointer=this;
    }
    /**
     * Invoke an option, setting its default value with a particular lexiccal cast function, a description and optionally a pointer to set to the result
     * \param option is the name of the option in the config or text files
     * \param defaultValue is the string representation of the default value as if a user typed it in
     * \param description is the textual description for the user when looking through the command line help
     * \param parser is the function that can convert a string to the Sirikata::Any of the appropriate type
     * \param changeFunction is the function that will be invoked if someone atomically changes the OptionValue type by invoking =
     * \param pointer holds a pointer to an OptionValue that will get set to the newly constructed class
     */
    OptionValue(const char* option, const char*defaultValue, const char* description, const std::tr1::function<Any(std::string)>& parser,  const std::tr1::function<void(const std::string&, Any, Any)> &changeFunction, OptionValue**pointer=NULL) {
        mParser=parser;
        mName=option;
        mDescription=description;
        mDefaultChar=defaultValue;
        mChangeFunction=changeFunction;
        if (pointer)
            *pointer=this;
    }



};
}
#endif //_SIRIKATA_OPTIONVALUE_HPP_

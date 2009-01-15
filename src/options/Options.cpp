/*     Iridium Configuration Options -- Iridium Options
 *  Options.cpp
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
#include "Options.hpp"
#include "boost/thread.hpp"
#include "boost/program_options.hpp"
namespace Iridium {
namespace {
//from boost
	    std::vector<std::string> splice_winmain(const std::string& input)
 	    {
 	        std::vector<std::string> result;
            using std::string;
 	        string::const_iterator i = input.begin(), e = input.end();
 	        for(;i != e; ++i)
 	            if (!isspace(*i))
 	                break;
 	       
 	        if (i != e) {
 	   
 	            std::string current;
 	            bool inside_quoted = false;
 	            int backslash_count = 0;
 	           
	            for(; i != e; ++i) {
 	                if (*i == '"') {
 	                    // '"' preceded by even number (n) of backslashes generates
                 // n/2 backslashes and is a quoted block delimiter
 	                    if (backslash_count % 2 == 0) {
 	                        current.append(backslash_count / 2, '\\');
 	                        inside_quoted = !inside_quoted;
 	                        // '"' preceded by odd number (n) of backslashes generates
 	                        // (n-1)/2 backslashes and is literal quote.
 	                    } else {
 	                        current.append(backslash_count / 2, '\\');               
 	                        current += '"';               
 	                    }
 	                    backslash_count = 0;
 	                } else if (*i == '\\') {
 	                    ++backslash_count;
 	                } else {
 	                    // Not quote or backslash. All accumulated backslashes should be
 	                    // added
 	                    if (backslash_count) {
 	                        current.append(backslash_count, '\\');
 	                        backslash_count = 0;
 	                    }
 	                    if (isspace(*i) && !inside_quoted) {
 	                        // Space outside quoted section terminate the current argument
 	                        result.push_back(current);
 	                        current.resize(0);
 	                        for(;i != e && isspace(*i); ++i)
 	                            ;
 	                        --i;
 	                    } else {                 
 	                        current += *i;
 	                    }
 	                }
 		        }
 	
 	            // If we have trailing backslashes, add them
 	            if (backslash_count)
 	                current.append(backslash_count, '\\');
 	       
 	            // If we have non-empty 'current' or we're still in quoted
 	            // section (even if 'current' is empty), add the last token.
 	            if (!current.empty() || inside_quoted)
 	                result.push_back(current);       
 	        }
 	        return result;
 	    }

}
class OptionRegistration {
public:
    static boost::mutex & OptionSetMutex() {
        static boost::mutex mutex;
        return mutex;
    }
    static void register_options(const std::map<std::string,OptionValue*>&names, boost::program_options::options_description &options) {
        for (std::map<std::string,OptionValue*>::const_iterator i=names.begin(),ie=names.end();
             i!=ie;
             ++i) {
            options.add_options()(i->second->mName,
                                  boost::program_options::value<std::string>()->default_value(i->second->defaultValue()),
                                  i->second->description());
        }
    }
    static void update_options(std::map<std::string,OptionValue*>&names, const boost::program_options::variables_map options) {
        for (std::map<std::string,OptionValue*>::iterator i=names.begin(),ie=names.end();
             i!=ie;
             ++i) {
            i->second->mValue.newAndDoNotFree(i->second->mParser(options[i->first].as<std::string>()));
        }
    }
};
bool OptionValue::initializationSet(const OptionValue&other) {
    if (mParser==NULL){
        mValue.newAndDoNotFree(other.mValue);
        mDefaultValue=other.mDefaultValue;
        mDescription=other.mDescription;
        mParser=other.mParser;
        mChangeFunction=other.mChangeFunction;
        mName=other.mName;
        return true;
    }else if (other.mParser==NULL) {
        return true;
    }else {
        return false;
    }
}

OptionValue& OptionValue::operator=(const OptionValue&other) {
    Any oldValue=mValue;
    mValue.newAndDoNotFree(other.mValue);
    mDefaultValue=other.mDefaultValue;
    mDescription=other.mDescription;
    mParser=other.mParser;
    mChangeFunction(mName,oldValue,mValue);
    mChangeFunction=other.mChangeFunction;
    mName=other.mName;
    return *this;
}
void OptionSet::parse(int argc, const char **argv){
    boost::program_options::options_description options;
    boost::program_options::variables_map output;
    {
        boost::unique_lock<boost::mutex> lock(OptionRegistration::OptionSetMutex());
        OptionRegistration::register_options(mNames,options);
    }
    boost::program_options::store( parse_command_line(argc, const_cast<char**>(argv), options), output);
    {
        boost::unique_lock<boost::mutex> lock(OptionRegistration::OptionSetMutex());
        OptionRegistration::update_options(mNames,output);
    }
}
void OptionSet::parse(const std::string&args){
    boost::program_options::options_description options;
    boost::program_options::variables_map output;
    {
        boost::unique_lock<boost::mutex> lock(OptionRegistration::OptionSetMutex());
        OptionRegistration::register_options(mNames,options);
    }
    std::vector<std::string> args_vec = splice_winmain(args);
    boost::program_options::store( boost::program_options::command_line_parser(args_vec).options(options).run(),output);
    {
        boost::unique_lock<boost::mutex> lock(OptionRegistration::OptionSetMutex());
        OptionRegistration::update_options(mNames,output);
    }
}
OptionSet* OptionSet::getOptionsNoLock(const std::string&s){
    std::map<std::string,OptionSet*>::iterator i=optionSets()->find(s);
    if (i==optionSets()->end()){
        return (*optionSets())[s]=new OptionSet;
    }else{
        return i->second;
    }
}
OptionSet* OptionSet::getOptions(const std::string&s){
    boost::unique_lock<boost::mutex> lock(OptionRegistration::OptionSetMutex());    
    return getOptionsNoLock(s);
}
OptionValue* OptionSet::referenceOption(const std::string&module, const std::string&option,OptionValue**pointer) {
    boost::unique_lock<boost::mutex> lock(OptionRegistration::OptionSetMutex());    
    return getOptionsNoLock(module)->referenceOptionNoLock(option,pointer);
}
OptionValue* OptionSet::referenceOptionNoLock(const std::string&option,OptionValue**pointer) {
    std::map<std::string,OptionValue*>::iterator where=mNames.find(option);
    if (where==mNames.end()) {
        OptionValue* newed=new OptionValue;
        if (pointer!=NULL)
            *pointer=newed;
        return mNames[option]=newed;
    }else {
        return where->second;
    }
}
OptionValue* OptionSet::referenceOption(const std::string&option,OptionValue**pointer) {    
    boost::unique_lock<boost::mutex> lock(OptionRegistration::OptionSetMutex());    
    return referenceOptionNoLock(option,pointer);
}
void OptionSet::addOptionNoLock(OptionValue*option) {
    std::map<std::string,OptionValue*>::iterator where=mNames.find(option->mName);
    if (where==mNames.end()) {
        mNames[option->mName]=option;
    }else {
        where->second->initializationSet(*option);
        delete option;
    }
}
void OptionSet::addOption(OptionValue *option) {
    boost::unique_lock<boost::mutex> lock(OptionRegistration::OptionSetMutex());
    addOptionNoLock(option);
}

InitializeOptions::InitializeOptions(const char * module,...) {
    va_list vl;
    va_start(vl,module);
    OptionValue* option;
    boost::unique_lock<boost::mutex> lock(OptionRegistration::OptionSetMutex());
    OptionSet* curmodule=OptionSet::getOptionsNoLock(module);
    while ((option=va_arg(vl,OptionValue*))!=NULL) {
        curmodule->addOptionNoLock(option);
    }
 
}
}

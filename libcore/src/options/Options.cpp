/*  Sirikata Configuration Options -- Sirikata Options
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

#include <sirikata/core/util/Standard.hh>
#include <sirikata/core/options/Options.hpp>
#include <boost/thread.hpp>
#include <boost/program_options.hpp>
#include <iostream>
#include <stdarg.h>
#include <sirikata/core/queue/ThreadSafeQueue.hpp>
#include <sirikata/core/util/DynamicLibrary.hpp>

namespace Sirikata {
class simple_string:public std::string {
public:
    simple_string(const std::string&other):std::string(other){

    }
/*
    char * data;
    unsigned int len;

    simple_string() {
        data=NULL;
        len=0;
    }
    simple_string(const std::string&s) {
        if (s.length()) {
            data=new char[s.length()];
            len=s.length();
            memcpy(data,s.data(),s.length());
        }else {
            data=NULL;
            len=0;
        }
    }
    simple_string(const std::vector<char>&other) {
        if (other.size()) {
            data= new char(other.size());
            memcpy(data,&*other.begin(),other.size());
            len=other.size();
        }else {
            len=0;
            data=NULL;
        }

    }
    simple_string(const simple_string&other) {
        if (other.len) {
            data=new char[other.len];
            len=other.len;
            memcpy(data,other.data,other.len);
        }else {
            len=0;
            data=NULL;
        }
    }
    simple_string&operator=(const simple_string&other) {
        if (other.len) {
            data=new char[other.len];
            len=other.len;
            memcpy(data,other.data,other.len);
        }else {
            len=0;
            data=NULL;
        }
        return *this;
    }
    ~simple_string() {
        delete []data;
    }
    operator std::string() {
        return std::string(data,len);
    }
*/
};
void validate(boost::any& v, const std::vector<String>& values, Sirikata::simple_string* target_type, int){
    using namespace boost;
    using namespace boost::program_options;

    // no previous instances
    validators::check_first_occurrence(v);
    // only one string in values
    v=any(simple_string(validators::get_single_string(values)));
}
}
/*
std::basic_ostream<char>& operator<< (std::basic_ostream<char>&is,const Sirikata::simple_string &output) {
    is<<output//std::string(output.data,output.len);
    return is;
}

std::istream &operator >>(std::istream &is,Sirikata::simple_string &output) {
    std::vector<char> test;
    while (!is.eof()) {
        char ch[1024];
        is.get(ch,1023);
        std::vector<char>::size_type adv=is.gcount();
        if (adv)
            test.insert(test.end(),ch,ch+adv);
    }
    output=Sirikata::simple_string(test);
    return is;
}
*/
namespace Sirikata {
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
namespace {
template <class T>class Stash{
    ThreadSafeQueue<std::pair<String,T*> > mQueue;
    class PairHasher{
    public:
        size_t operator()(const std::pair<String,T*>&toBeHashed)const{
            return std::tr1::hash<T*>()(toBeHashed.second);
        }
    };
public:
    void hideUntilQuit(String s, T*ov) {
        if (ov!=NULL) {
            mQueue.push(std::pair<String,T*>(s,ov));
        }
    }
    void purgeUnused() {
        std::tr1::unordered_set<std::pair<String,T*>, PairHasher > alreadyDestroyed;
        std::deque<std::pair<String,T*> > toDestroy;
        mQueue.swap(toDestroy);
        while(!toDestroy.empty()) {
            if (alreadyDestroyed.find(toDestroy.front())==alreadyDestroyed.end()) {
                alreadyDestroyed.insert(toDestroy.front());
                //printf ("Destroying %s\n",toDestroy.front().first.c_str());
                delete toDestroy.front().second;
                toDestroy.pop_front();
            }else {
                toDestroy.pop_front();
            }
        }
    }
    ~Stash() {
        purgeUnused();
    }
};
class HolderStash:public Stash<Any::Holder>, public AutoSingleton<HolderStash> {};
class ValueStash:public Stash<OptionValue>, public AutoSingleton<ValueStash> {};
class QuitOptionsPlugin {
public:
    ~QuitOptionsPlugin() {
        for (std::map<OptionSet::StringVoid,OptionSet*>::iterator i=OptionSet::optionSets()->begin();
             i!=OptionSet::optionSets()->end();
             ++i) {
            delete i->second;
        }
        delete OptionSet::optionSets();
        HolderStash::destroy();
        ValueStash::destroy();
        Sirikata::DynamicLibrary::gc();
    }
}gQuitOptionsPlugin;
}
AUTO_SINGLETON_INSTANCE(HolderStash);
AUTO_SINGLETON_INSTANCE(ValueStash);
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
                                  boost::program_options::value<simple_string>()->default_value(simple_string(i->second->defaultValue())),
                                  i->second->description());
        }
    }
    static bool update_options(std::map<std::string,OptionValue*>&names, boost::program_options::options_description &options_description, const boost::program_options::variables_map options, bool use_defaults) {
        if (options.count("help")){
            try {
                std::cout << options_description;
            }
            catch (...) {
                std::cout << "Ran out of options to display!" << std::endl;
                return false;
            }
            return false;
        }
        if (options.count("TESTO"))
            assert(false);
        for (std::map<std::string,OptionValue*>::iterator i=names.begin(),ie=names.end();
             i!=ie;
             ++i) {
            if (options.count(i->first) && (use_defaults || !options[i->first].defaulted())) {
                     const simple_string* s=boost::any_cast<simple_string>(&options[i->first].value());
                     assert(s!=NULL);
                     HolderStash::getSingleton().hideUntilQuit(i->first,i->second->mValue.newAndDoNotFree(i->second->mParser(*s)));
				 }
        }
        return true;
    }
};
bool OptionSet::initializationSet(OptionValue* thus, const OptionValue&other) {
    if (thus->mParser==NULL){

        HolderStash::getSingleton().hideUntilQuit(thus->mName,thus->mValue.newAndDoNotFree(other.mValue));
        thus->mDefaultChar=other.mDefaultChar;
        thus->mDefaultValue=other.mDefaultValue;
        thus->mDescription=other.mDescription;
        thus->mParser=other.mParser;
        thus->mChangeFunction=other.mChangeFunction;
        thus->mName=other.mName;
        return true;
    }else if (other.mParser==NULL) {
        return true;
    }else {
        return false;
    }
}

OptionValue& OptionValue::operator=(const OptionValue&other) {
    Any oldValue=mValue;
    HolderStash::getSingleton().hideUntilQuit(mName,mValue.newAndDoNotFree(other.mValue));
    mDefaultChar=other.mDefaultChar;
    mDefaultValue=other.mDefaultValue;
    mDescription=other.mDescription;
    mParser=other.mParser;
    mChangeFunction(mName,oldValue,mValue);
    mChangeFunction=other.mChangeFunction;
    mName=other.mName;
    return *this;
}
OptionSet::OptionSet() {
    mParsingStage=PARSED_NO_OPTIONS;
}
OptionSet::~OptionSet() {
    for (std::map<std::string,OptionValue*>::iterator i=mNames.begin();
         i!=mNames.end();
         ++i){
        ValueStash::getSingleton().hideUntilQuit(i->first,i->second);
    }
}
void OptionSet::parse(int argc, const char * const *argv, bool use_defaults){
    if (argc>1)
        mParsingStage=PARSED_UNBLANK_OPTIONS;
    boost::program_options::options_description options;
    boost::program_options::variables_map output;
    {
        boost::unique_lock<boost::mutex> lock(OptionRegistration::OptionSetMutex());
        OptionRegistration::register_options(mNames,options);
    }
    options.add_options()("help","Print available options");
    boost::program_options::store( parse_command_line(argc, const_cast<char**>(argv), options), output);
    bool dienow=false;
    {
        boost::unique_lock<boost::mutex> lock(OptionRegistration::OptionSetMutex());
        dienow=!OptionRegistration::update_options(mNames,options,output, use_defaults);
    }
    if (dienow)
        exit(0);
}
void OptionSet::parseFile(const std::string& file, bool required, bool use_defaults) {
    mParsingStage=PARSED_UNBLANK_OPTIONS;
    boost::program_options::options_description options;
    boost::program_options::variables_map output;
    {
        boost::unique_lock<boost::mutex> lock(OptionRegistration::OptionSetMutex());
        OptionRegistration::register_options(mNames,options);
    }
    options.add_options()("help","Print available options");
    std::ifstream cf_fp(file.c_str());
    if (!cf_fp) {
        if (required) exit(0);
        return;
    }
    boost::program_options::store( boost::program_options::parse_config_file(cf_fp, options), output);
    cf_fp.close();
    bool dienow=false;
    {
        boost::unique_lock<boost::mutex> lock(OptionRegistration::OptionSetMutex());
        dienow=!OptionRegistration::update_options(mNames,options,output, use_defaults);
    }
    if (dienow)
        exit(0);
}
void OptionSet::parse(const std::string&args, bool use_defaults){
    if (args.size())
        mParsingStage=PARSED_UNBLANK_OPTIONS;
    boost::program_options::options_description options;
    boost::program_options::variables_map output;
    {
        boost::unique_lock<boost::mutex> lock(OptionRegistration::OptionSetMutex());
        OptionRegistration::register_options(mNames,options);
    }
    options.add_options()("help","Print available options");
    std::vector<std::string> args_vec = splice_winmain(args);
    boost::program_options::store( boost::program_options::command_line_parser(args_vec).options(options).run(),output);
    bool dienow=false;
    {
        boost::unique_lock<boost::mutex> lock(OptionRegistration::OptionSetMutex());
        dienow=!OptionRegistration::update_options(mNames,options,output, use_defaults);
    }
    if (dienow)
        exit(0);
}
OptionSet* OptionSet::getOptionsNoLock(const std::string&s, const void * context){
    std::map<StringVoid,OptionSet*>::iterator i=optionSets()->find(StringVoid(s,context));
    if (i==optionSets()->end()){
        return (*optionSets())[StringVoid(s,context)]=new OptionSet;
    }else{
        return i->second;
    }
}
OptionSet* OptionSet::getOptions(const std::string&s, const void * context){
    boost::unique_lock<boost::mutex> lock(OptionRegistration::OptionSetMutex());
    return getOptionsNoLock(s,context);
}
OptionSet*OptionSet::getOptions(const std::string&s){
    return getOptions(s,NULL);
}
OptionSet*OptionSet::getOptions(){
    return getOptions("",NULL);
}
OptionValue* OptionSet::referenceOption(const std::string&module, const std::string&option,OptionValue**pointer) {
    boost::unique_lock<boost::mutex> lock(OptionRegistration::OptionSetMutex());
    return getOptionsNoLock(module,NULL)->referenceOptionNoLock(option,pointer);
}
OptionValue* OptionSet::referenceOption(const std::string&module, const void * thus, const std::string&option,OptionValue**pointer) {
    boost::unique_lock<boost::mutex> lock(OptionRegistration::OptionSetMutex());
    return getOptionsNoLock(module,thus)->referenceOptionNoLock(option,pointer);
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
        if (where->second != option) {
            OptionSet::initializationSet(where->second,*option);
            delete option;
        }
    }
}
void OptionSet::addOption(OptionValue *option) {
    boost::unique_lock<boost::mutex> lock(OptionRegistration::OptionSetMutex());
    addOptionNoLock(option);
}

InitializeClassOptions::InitializeClassOptions(const char * module,const void * thus,...) {
    va_list vl;
    va_start(vl,thus);
    OptionValue* option;
    OptionSet* curmodule;
    {
        boost::unique_lock<boost::mutex> lock(OptionRegistration::OptionSetMutex());
        curmodule=OptionSet::getOptionsNoLock(module,thus);
        while ((option=va_arg(vl,OptionValue*))!=NULL) {
            curmodule->addOptionNoLock(option);
        }
    }
    if (curmodule->mParsingStage==OptionSet:: PARSED_NO_OPTIONS||curmodule->mParsingStage==OptionSet:: PARSED_BLANK_OPTIONS) {
        curmodule->parse("");
    }
    if (curmodule->mParsingStage==OptionSet:: PARSED_UNBLANK_OPTIONS) {
        std::cerr<< "Error adding more options after options have been parsed"<<std::endl;
        curmodule->mParsingStage=OptionSet:: PARSED_PARTIAL_UNBLANK_OPTIONS;
    }
}

InitializeClassOptions InitializeClassOptions::module(const char * module) {
    OptionSet* opt_set = OptionSet::getOptions(module);
    return InitializeClassOptions(opt_set);
}

InitializeClassOptions::InitializeClassOptions(OptionSet* opt_set)
 : mOptionSet(opt_set)
{
}

InitializeGlobalOptions::InitializeGlobalOptions(const char * module,...):InitializeClassOptions(OptionSet::getOptionsNoLock(module,NULL)) {
    va_list vl;
    va_start(vl,module);
    OptionValue* option;
    OptionSet* curmodule;
    {
        boost::unique_lock<boost::mutex> lock(OptionRegistration::OptionSetMutex());
        curmodule=OptionSet::getOptionsNoLock(module,NULL);
        while ((option=va_arg(vl,OptionValue*))!=NULL) {
            curmodule->addOptionNoLock(option);
        }
    }
    if (curmodule->mParsingStage==OptionSet:: PARSED_NO_OPTIONS||curmodule->mParsingStage==OptionSet:: PARSED_BLANK_OPTIONS) {
        curmodule->parse("");
    }
    if (curmodule->mParsingStage==OptionSet:: PARSED_UNBLANK_OPTIONS) {
        std::cerr<< "Error adding more options after options have been parsed"<<std::endl;
        curmodule->mParsingStage=OptionSet:: PARSED_PARTIAL_UNBLANK_OPTIONS;
    }
}

InitializeClassOptions InitializeClassOptions::addOption(OptionValue* opt_value) {
    assert(opt_value != NULL);
    mOptionSet->addOption(opt_value);
    return InitializeClassOptions(mOptionSet);
}

}

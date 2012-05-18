/*  Sirikata Configuration Options -- Sirikata Options
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

/*
 *  EXAMPLE OPTIONS
 *
 *
 * extern OptionValue * SHIELD_ENERGY=NULL;
 * static InitializeOptions o("",
 *                            addOption("ShieldOption",2,"Sets the awesome option",&SHIELD_ENERGY),
 *                            addOption("server",IPAddress(127,0,0,1,24)),
 *                            addOption("client",IPAddress(127,0,0,1,24)),
 *                            addOption("server",IPAddress(127,0,0,1,24)),
 *                            addOption("server",IPAddress(127,0,0,1,24))
 *                            NULL);
 * extern OptionValue SHIELD_OPTION = referenceOption(CURRENT_MODULE,"ShieldOption");
 *
 *
 */

#ifndef _SIRIKATA_OPTIONS_HPP_
#define _SIRIKATA_OPTIONS_HPP_

#include <sirikata/core/util/Platform.hpp>

namespace Sirikata {

#define GLOBAL_OPTIONS_MODULE ""
#define SIRIKATA_OPTIONS_MODULE "sirikata"

/**
 * A dummy class to statically initialize a bunch of option classes that could add to a module
 */
class SIRIKATA_EXPORT InitializeClassOptions {
    public:
        // Takes a null terminated arg tuple of OptionValues* that should be added to the option set
        InitializeClassOptions(const char *, const void * thus, ...);
        static InitializeClassOptions module(const char* module);
        InitializeClassOptions addOption(OptionValue* opt_value);
    protected:
        InitializeClassOptions(OptionSet* opt_set);

        OptionSet* mOptionSet;
};

/**
 * A dummy class to statically initialize a bunch of option classes that could add to a module
 */
class SIRIKATA_EXPORT InitializeGlobalOptions : public InitializeClassOptions {
    public:
        ///Takes a null terminated arg tuple of OptionValues* that should be added to the option set
        InitializeGlobalOptions(const char *,...);
};

/**
 * This class holds a set of options that may appear on a command line or within an argument to a module
 * Holds a static index to all OptionSets currently available in the program.
 */
class SIRIKATA_EXPORT OptionSet {

public:

    class StringVoid {
        String mString;
        const void* mVoid;

        public:
            StringVoid(const String& ss, const void* vv) {
                mString = ss;
                mVoid = vv;
            }
            bool operator < (const StringVoid& other) const {
                return (mString == other.mString ? mVoid < other.mVoid : mString < other.mString);
            }
            bool operator == (const StringVoid& other) const {
                return (mString == other.mString && mString == other.mString);
            }
            const String& getString() const {
                return mString;
            }
            const void* getVoid() const {
                return mVoid;
            }
    };

    typedef std::map<String, OptionValue*> OptionNameMap;
    typedef std::map<StringVoid, OptionSet*> NamedOptionSetMap;
    typedef NamedOptionSetMap* NamedOptionSetMapPtr;

    OptionSet();
    ~OptionSet();
    void parse(const String&, bool use_defaults = true, bool missing_only = false);
    void parse(int, const char* const*, bool use_defaults = true, bool missing_only = false);
    void parseFile(const String&, bool required, bool use_defaults = true, bool missing_only = false);
    /// Fills in defaults for any options that didn't already have values filled
    /// in. Useful if you add options and need to fill in defaults but can't
    /// parse with defaults since that would overwrite already-parsed options.
    void fillMissingDefaults();
    void addOption(OptionValue* v);
    const OptionNameMap& getOptionsMap() const {
        return mNames;
    }
    OptionValue* referenceOption(const String& option, OptionValue** pointer = NULL);

    static OptionValue* referenceOption(const String& module, const String& option, OptionValue** pointer = NULL);
    static OptionValue* referenceOption(const String& module, const void* context_ptr, const String& option, OptionValue** pointer = NULL);

    static NamedOptionSetMapPtr optionSets() {
        static NamedOptionSetMapPtr retval = new NamedOptionSetMap();
        return retval;
    }

    static OptionSet* getOptions(const String& s, const void* context);
    static OptionSet* getOptions(const String& s);
    static OptionSet* getOptions();

protected:

    OptionNameMap mNames;
    friend class InitializeGlobalOptions;
    friend class InitializeClassOptions;
    void addOptionNoLock(OptionValue*);
    static OptionSet* getOptionsNoLock(const String&s, const void * context);
    OptionValue* referenceOptionNoLock(const String &option, OptionValue** pointer);

    class OptionNameAndContext {
        const void* mContext;
        String mName;

        public:
            OptionNameAndContext(const String& nam, const void* con) {
                mContext = con;
                mName = nam;
            }

            bool operator< (const OptionNameAndContext& other) const {
                if (other.mContext == mContext)
                    return mName < other.mName;
                return other.mContext < mContext;
            }
    };

    enum ParsingStage {
        PARSED_NO_OPTIONS,
        PARSED_BLANK_OPTIONS,
        PARSED_UNBLANK_OPTIONS,
        PARSED_PARTIAL_UNBLANK_OPTIONS
    } mParsingStage;

    bool initializationSet(OptionValue* thus, const OptionValue& other);

};

}

#endif //_SIRIKATA_OPTIONS_HPP_

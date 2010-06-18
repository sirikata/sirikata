/*  Sirikata Tests -- Sirikata Test Suite
 *  SQLiteMinitransactionTest.hpp
 *
 *  Copyright (c) 2009, Daniel Reiter Horn and Ewen Cheslack-Postava
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
#include <cxxtest/TestSuite.h>
#include <sirikata/core/persistence/ObjectStorage.hpp>
#include <sirikata/core/util/PluginManager.hpp>
#include <sirikata/core/util/DynamicLibrary.hpp>
#include <sirikata/core/persistence/MinitransactionHandlerFactory.hpp>
#include "MinitransactionHandlerTest.hpp"
namespace MinitransactionTestNs {
extern const char *databaseMinitransactionalFilename;
void setupMinitransactionalHandler();
void teardownMinitransactionalHandler();
}

class SQLiteMinitransactionTest:public CxxTest::TestSuite
{
public:
    static Sirikata::Persistence::MinitransactionHandler* createMinitransactionalHandlerFunction(const Sirikata::String&s){
        Sirikata::String arg=s;
        if (arg.find("--databasefile")==Sirikata::String::npos) {
            arg="--databasefile "+Sirikata::String(MinitransactionTestNs::databaseMinitransactionalFilename)+arg;
        }
        return Sirikata::Persistence::MinitransactionHandlerFactory::getSingleton().getConstructor("sqlite")(arg);
    }
    Sirikata::Persistence::MinitransactionHandler*mDatabase;
    SQLiteMinitransactionTest() {
        Sirikata::PluginManager plugins;
        plugins.load("sqlite");
        mDatabase=createMinitransactionalHandlerFunction("");
    }
    ~SQLiteMinitransactionTest(){
        if (mDatabase)
            delete mDatabase;
    }
    static SQLiteMinitransactionTest*createSuite() {
        return new SQLiteMinitransactionTest;
    }
    static void destroySuite(SQLiteMinitransactionTest*mt) {
        delete mt;
    }
    void testMinitransactionHandlerOrder( void ) {
        test_minitransaction_handler_order(&MinitransactionTestNs::setupMinitransactionalHandler,
                                           &SQLiteMinitransactionTest::createMinitransactionalHandlerFunction,
                                           "",
                                           &MinitransactionTestNs::teardownMinitransactionalHandler);
    }

    void xestStressMinitransactionHandlerOrder( void ) {
        stress_test_minitransaction_handler(&MinitransactionTestNs::setupMinitransactionalHandler,
                                            &SQLiteMinitransactionTest::createMinitransactionalHandlerFunction,
                                            "",
                                            &MinitransactionTestNs::teardownMinitransactionalHandler,
                                            10,
                                            1);
    }
    void xestStressMinitransactionHandlerOrder2( void ) {
        stress_test_minitransaction_handler(&MinitransactionTestNs::setupMinitransactionalHandler,
                                            &SQLiteMinitransactionTest::createMinitransactionalHandlerFunction,
                                            "",
                                            &MinitransactionTestNs::teardownMinitransactionalHandler,
                                            10,
                                            2);
    }
    void xestStressMinitransactionHandlerOrder10( void ) {
        stress_test_minitransaction_handler(&MinitransactionTestNs::setupMinitransactionalHandler,
                                            &SQLiteMinitransactionTest::createMinitransactionalHandlerFunction,
                                            "",
                                            &MinitransactionTestNs::teardownMinitransactionalHandler,
                                            100,
                                            10);
    }

};

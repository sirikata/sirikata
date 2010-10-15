/*  Sirikata
 *  Test.cpp
 *
 *  Copyright (c) 2009, Ewen Cheslack-Postava
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

#include <cxxtest/TestRunner.h>
#include <cxxtest/TestListener.h>
#include <stdio.h>
#include <string.h>

namespace Sirikata {

using namespace CxxTest;

class StandardTestListener : public TestListener {
public:
    StandardTestListener() {
    }
    virtual ~StandardTestListener() {
    }

    int run(const char * singleSuite) {

    	bool foundTest = false;

    	if (singleSuite != NULL) {
			RealWorldDescription wd;

			tracker().enterWorld( wd );
			enterWorld( wd );
			if ( wd.setUp() ) {
				for ( SuiteDescription *sd = wd.firstSuite(); sd; sd = sd->next() )
					if ( sd->active() && strcmp(singleSuite, sd->suiteName())==0 ) {

						foundTest = true;
						printf("\n\n========================================\n");
						printf("RUNNING SINGLE SUITE\n%s\n", singleSuite);
						printf("========================================");

			            tracker().enterSuite( *sd );
			            enterSuite( *sd );
			            if ( sd->setUp() ) {
			                for ( TestDescription *td = sd->firstTest(); td; td = td->next() )
			                    if ( td->active() ) {

			                        tracker().enterTest( *td );
			                        enterTest( *td );
			                        if ( td->setUp() ) {
			                            td->run();
			                            td->tearDown();
			                        }
			                        tracker().leaveTest( *td );
			                        leaveTest( *td );

			                    }

			                sd->tearDown();
			            }
			            tracker().leaveSuite( *sd );

					}
				wd.tearDown();
			}
			tracker().leaveWorld( wd );
			leaveWorld( wd );

    	}
    	if (singleSuite==NULL || !foundTest) {
			printf("\n\n========================================\n");
			printf("RUNNING ALL SUITES\n");
			printf("========================================\n\n");

    		TestRunner::runAllTests( *this );
    	}

		printf("\n\n========================================\n");
		printf("SUMMARY: %d failed, %d warnings.\n",
			tracker().failedTests(), tracker().warnings());
		printf("========================================\n");
		return tracker().failedTests();
    }

    virtual void process_commandline(int& /*argc*/, char** /*argv*/) {}

    virtual void enterWorld( const WorldDescription & /*desc*/ ) {}
    virtual void enterSuite( const SuiteDescription & desc ) {
    }
    virtual void enterTest( const TestDescription & desc ) {
    	printf("\n\n========================================\n");
        printf("BEGIN TEST %s.%s\n", desc.suiteName(), desc.testName());
    }
    virtual void trace( const char * /*file*/, unsigned /*line*/,
        const char * /*expression*/ ) {}
    virtual void warning( const char * file, unsigned line, const char * expression ) {
        printf("Warning: At %s:%d: %s\n", file, line, expression);
    }
    virtual void failedTest( const char * file, unsigned line, const char * expression ) {
        printf("Error: At %s:%d: %s\n", file, line, expression);
    }
    virtual void failedAssert( const char * file, unsigned line, const char * expression ) {
        printf("Error: Assert failed at %s:%d: %s\n", file, line, expression);
    }
    virtual void failedAssertEquals( const char * file, unsigned line, const char * xStr,
        const char * yStr, const char * x, const char * y ) {
        printf("Error: Assert failed at %s:%d: (%s == %s), was (%s == %s)\n", file, line, xStr, yStr, x, y);
    }
    virtual void failedAssertSameData( const char * file, unsigned line, const char * xStr,
        const char * yStr, const char * sizeStr, const void * x, const void * y, unsigned size) {
        printf("Error: Assert failed at %s:%d: (%s same as %s, size = %s)\n", file, line, xStr, yStr, sizeStr);
    }
    virtual void failedAssertDelta( const char * file, unsigned line, const char * xStr,
        const char * yStr, const char * dStr, const char * x, const char * y, const char * d) {
        printf("Error: Assert failed at %s:%d: (%s == %s, delta = %s), was (%s == %s, delta = %s)\n", file, line, xStr, yStr, dStr, x, y, d);
    }
    virtual void failedAssertDiffers( const char * file, unsigned line, const char * xStr,
        const char * yStr, const char * value) {
        printf("Error: Assert failed at %s:%d: (%s != %s), value: %s\n", file, line, xStr, yStr, value);
    }
    virtual void failedAssertLessThan( const char * /*file*/, unsigned /*line*/,
        const char * /*xStr*/, const char * /*yStr*/,
        const char * /*x*/, const char * /*y*/ ) {}
    virtual void failedAssertLessThanEquals( const char * /*file*/, unsigned /*line*/,
        const char * /*xStr*/, const char * /*yStr*/,
        const char * /*x*/, const char * /*y*/ ) {}
    virtual void failedAssertPredicate( const char * /*file*/, unsigned /*line*/,
        const char * /*predicate*/, const char * /*xStr*/, const char * /*x*/ ) {}
    virtual void failedAssertRelation( const char * /*file*/, unsigned /*line*/,
        const char * /*relation*/, const char * /*xStr*/, const char * /*yStr*/,
        const char * /*x*/, const char * /*y*/ ) {}
    virtual void failedAssertThrows( const char * /*file*/, unsigned /*line*/,
        const char * /*expression*/, const char * /*type*/,
        bool /*otherThrown*/ ) {}
    virtual void failedAssertThrowsNot( const char * /*file*/, unsigned /*line*/,
        const char * /*expression*/ ) {}
    virtual void failedAssertSameFiles( const char* , unsigned , const char* , const char*, const char* ) {}
    virtual void leaveTest( const TestDescription & desc ) {
    	printf("END TEST %s.%s\n", desc.suiteName(), desc.testName());
    	printf("========================================\n");
    }
    virtual void leaveSuite( const SuiteDescription & /*desc*/ ) {}
    virtual void leaveWorld( const WorldDescription & /*desc*/ ) {}
};

} // namespace Sirikata


int main(int argc, const char * argv [])
{
	if(argc > 1) {
		return Sirikata::StandardTestListener().run(argv[1]);
	} else {
		return Sirikata::StandardTestListener().run(NULL);
	}
}

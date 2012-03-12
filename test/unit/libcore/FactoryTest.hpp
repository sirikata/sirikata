/*  Sirikata Tests -- Sirikata Test Suite
 *  FactoryTest.hpp
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
#include <cxxtest/TestSuite.h>
#include <sirikata/core/util/Factory.hpp>

class FactoryTest :  public CxxTest::TestSuite {
public:
    class Test{
    public:
        virtual ~Test(){}
        virtual int getA(){
            return 0;
        }
        virtual int getB(){
            return 0;
        }
        virtual int getC(){
            return 0;
        }
        virtual int getD(){
            return 0;
        }
        virtual int getE(){
            return 0;
        }
        virtual int getF(){
            return 0;
        }
    };
    class Test01:public Test {
        int mA;
    public:
        Test01() {mA=1;}
        Test01(int a) {mA=a;}
        virtual int getA(){
            return mA;
        }

    };

    class Test2:public Test {
        int mA;
        int mB;
    public:
        Test2(int a,int b) {mA=a;mB=b;}
        virtual int getA(){
            return mA;
        }
        virtual int getB(){
            return mB;
        }

    };
    void setUp( void )
    {
    }
    void tearDown( void )
    {
    }
    static Test*f0func() {
        return new Test01;
    }
    static Test*f1func(int arg) {
        return new Test01(arg);
    }
    static Test*f2func(int arga, int argb) {
        return new Test2(arga,argb);
    }

    static std::tr1::shared_ptr<Test>f0Sfunc() {
        return std::tr1::shared_ptr<Test>(new Test01);
    }
    static std::tr1::shared_ptr<Test>f1Sfunc(int arg) {
        return std::tr1::shared_ptr<Test>(new Test01(arg));
    }
    static std::tr1::shared_ptr<Test>f2Sfunc(int arga, int argb) {
        return std::tr1::shared_ptr<Test>(new Test2(arga,argb));
    }
    void testFactory( void ) {
        using namespace Sirikata;
        Factory<Test*> f0;
        Factory1<Test*,int> f1;
        Factory2<Test*,int,int> f2;
        f0.registerConstructor("0",&FactoryTest::f0func);
        Test* a=f0.getConstructor("0")();
        TS_ASSERT_EQUALS(a->getA(),1);
        delete a;
        f1.registerConstructor("1",&FactoryTest::f1func);
        a=f1.getConstructor("1")(2);
        TS_ASSERT_EQUALS(a->getA(),2);
        delete a;
        f2.registerConstructor("2",&FactoryTest::f2func);
        a=f2.getConstructor("2")(3,4);
        TS_ASSERT_EQUALS(a->getA(),3);
        TS_ASSERT_EQUALS(a->getB(),4);
        delete a;
    }

    void testSharedFactory( void ) {
        using namespace Sirikata;
        using namespace std::tr1;
        Factory<shared_ptr<Test> > f0;
        Factory1<shared_ptr<Test> ,int> f1;
        Factory2<shared_ptr<Test> ,int,int> f2;
        f0.registerConstructor("0",&FactoryTest::f0Sfunc);
        shared_ptr<Test> a(f0.getConstructor("0")());
        TS_ASSERT_EQUALS(a->getA(),1);
        f1.registerConstructor("1",&FactoryTest::f1Sfunc);
        shared_ptr<Test> b(f1.getConstructor("1")(2));
        a=b;
        TS_ASSERT_EQUALS(a->getA(),2);
        f2.registerConstructor("2",&FactoryTest::f2Sfunc);
        shared_ptr<Test> c(f2.getConstructor("2")(3,4));
        a=c;
        TS_ASSERT_EQUALS(a->getA(),3);
        TS_ASSERT_EQUALS(a->getB(),4);
    }

};

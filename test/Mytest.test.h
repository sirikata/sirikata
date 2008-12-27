#include <cxxtest/TestSuite.h>
#include <iostream>
class MyTestSuite : public CxxTest::TestSuite 
{
public:
    static MyTestSuite * createSuite( void ) {
        MyTestSuite * mts=new MyTestSuite();
        std::cout << "\nMaking a new suite 0x" << (intptr_t)mts << std::endl;
        return mts;
    }
    static void destroySuite(MyTestSuite * k) {
        std::cout << "Destroying suite 0x" << (intptr_t)k << std::endl;
        delete k;        
    }
    void setUp( void )
    {
        std::cout << "SETTING UP 0x" << (intptr_t)this << std::endl;
    }
    void tearDown( void ) 
    {
        std::cout << "Mr Gorbachev, tear down 0x" << (intptr_t)this << " wall" << std::endl;
    }
    void testAddition( void )
    {
        TS_ASSERT( 1 + 1 > 1 );
        TS_ASSERT_EQUALS( 1 + 1, 2 );
    }
    void testMultiplication( void )
    {
        TS_ASSERT( 1 * 1 > 0 );
        TS_ASSERT_EQUALS( 1 * 1, 1 );
    }
};


class MyOtherTestSuite : public CxxTest::TestSuite 
{
public:
    static MyOtherTestSuite * createSuite( void ) {
        MyOtherTestSuite * mots=new MyOtherTestSuite();
        std::cout << "\nyMaking a new suite 0x" << (intptr_t)mots << std::endl;
        return mots;
    }
    static void destroySuite(MyOtherTestSuite * k) {
        std::cout << "yDestroying suite 0x" << (intptr_t)k << std::endl;
        delete k;        
    }
    void setUp( void )
    {
        std::cout << "ySETTING UP 0x" << (intptr_t)this << std::endl;
    }
    void tearDown( void ) 
    {
        std::cout << "yMr Gorbachev, tear down 0x" << (intptr_t)this << " wall" << std::endl;
    }
    void testAddition( void )
    {
        TS_ASSERT( 1 + 1 > 1 );
        TS_ASSERT_EQUALS( 1 + 1, 2 );
    }
    void testMultiplication( void )
    {
        TS_ASSERT( 1 * 1 == 1 );
        TS_ASSERT_EQUALS( 1 * 1, 1 );
    }
};

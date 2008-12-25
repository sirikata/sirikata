#include <cxxtest/TestSuite.h>
#include <stdio.h>
class MyTestSuite : public CxxTest::TestSuite 
{
public:
    static MyTestSuite * createSuite( void ) {
        MyTestSuite * mts=new MyTestSuite();
        printf ("\nMaking a new suite 0x%x\n",mts);
        return mts;
    }
    static void destroySuite(MyTestSuite * k) {
        printf ("Destroying suite 0x%x\n",k);
        delete k;        
    }
    void setUp( void )
    {
        printf ("SETTING UP 0x%x\n",this);
    }
    void tearDown( void ) 
    {
        printf ("Mr Gorbachev, tear down 0x%x wall\n",this);
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
        printf ("\nyMaking a new suite 0x%x\n",mots);
        return mots;
    }
    static void destroySuite(MyOtherTestSuite * k) {
        printf ("yDestroying suite 0x%x\n",k);
        delete k;        
    }
    void setUp( void )
    {
        printf ("ySETTING UP 0x%x\n",this);
    }
    void tearDown( void ) 
    {
        printf ("yMr Gorbachev, tear down 0x%x wall\n",this);
    }
    void testAddition( void )
    {
        TS_ASSERT( 1 + 1 > 1 );
        TS_ASSERT_EQUALS( 1 + 1, 2 );
    }
    void testMultiplication( void )
    {
        TS_ASSERT( 1 * 1 == 0 );
        TS_ASSERT_EQUALS( 1 * 1, 1 );
    }
};

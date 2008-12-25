#include <cxxtest/TestSuite.h>
#include <stdio.h>
class xMyTestSuite : public CxxTest::TestSuite 
{
public:
    static xMyTestSuite * createSuite( void ) {
        xMyTestSuite * mts=new xMyTestSuite();
        printf ("\nMaking a new suite 0x%x\n",mts);
        return mts;
    }
    static void destroySuite(xMyTestSuite * k) {
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


class xMyOtherTestSuite : public CxxTest::TestSuite 
{
public:
    static xMyOtherTestSuite * createSuite( void ) {
        xMyOtherTestSuite * mots=new xMyOtherTestSuite();
        printf ("\nyMaking a new suite 0x%x\n",mots);
        return mots;
    }
    static void destroySuite(xMyOtherTestSuite * k) {
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

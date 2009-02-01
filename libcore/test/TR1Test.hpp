#include "util/Platform.hh"
#include <cxxtest/TestSuite.h>
class TR1Test : public CxxTest::TestSuite
{
public:
    void testUnordered (void )
    {
        using namespace tech;  
        typedef unordered_map<std::string, unsigned long> Map;
        typedef unordered_set<std::string> Set;
        typedef unordered_map<unsigned long,std::string> RMap;
        Map colors;
        RMap rcolors;
        Set colornames;
        colornames.insert("black");
        colors["black"] = 0x000000ul;
        colornames.insert("red");
        colors["red"] = 0xff0000ul;
        colornames.insert("white");
        colors.insert(Map::value_type("white",0xfffffful));
        colornames.insert("green");
        colors["green"] = 0x00ff00ul;
        colornames.insert("blue");
        colors["blue"] = 0x0000fful;
        colornames.insert("blue");
        colors["blue"] = 0x0000fful;//again just for kicks
        TS_ASSERT_EQUALS(colors["red"],0xff0000ul);
        TS_ASSERT_EQUALS(colors["black"],0x000000ul);
        TS_ASSERT_EQUALS(colors["white"],0xfffffful);
        TS_ASSERT_EQUALS(colors["green"],0x00ff00ul);
        TS_ASSERT_EQUALS(colors["blue"],0x0000fful);
        for (Map::iterator i = colors.begin(); 
             i != colors.end(); 
             ++i) {
            rcolors[i->second]=i->first;
            switch (i->second) {
              case 0xff0000ul:
                TS_ASSERT_EQUALS(i->first,"red");
                break;
              case 0x00ff00ul:
                TS_ASSERT_EQUALS(i->first,"green");
                break;
              case 0x0000fful:
                TS_ASSERT_EQUALS(i->first,"blue");
                break;
              case 0x000000ul:
                TS_ASSERT_EQUALS(i->first,"black");
                break;
              case 0xfffffful:
                TS_ASSERT_EQUALS(i->first,"white");
                break;
              default:
                TS_FAIL("Color not matched");
                break;
            }
        }
        for (Set::iterator i=colornames.begin();i!=colornames.end();++i) {
            TS_ASSERT(*i=="red"||*i=="white"||*i=="black"||*i=="blue"||*i=="green");
        }
        TS_ASSERT_EQUALS((int)colornames.size(),5);
        TS_ASSERT_EQUALS((int)colors.size(),5);
        TS_ASSERT_EQUALS((int)rcolors.size(),5);
        TS_ASSERT_DIFFERS(colors.find("white"),colors.end());
        colors.erase(colors.find("white"));
        TS_ASSERT_EQUALS(colors.find("white"),colors.end());
        TS_ASSERT_EQUALS((int)colors.size(),4);
    }
};
